#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierUtilisateur.h" // pour la gestion des Clients

int idQ,idShm,idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;
char* pShm;

void afficheTab();

void ajouterUtilisateurDansTable(MESSAGE m) ;
void handleSIGINT(int sig);
void HandlerSIGCHLD(int sig); 

sigjmp_buf contexte;


int main()
{
  // Armement des signaux
  // TO DO
  fprintf(stderr, "(SERVEUR %d) Armement du signal SIGINT\n", getpid());
    
  struct sigaction C;
  C.sa_handler = handleSIGINT; // Définir le gestionnaire pour SIGINT
  C.sa_flags = 0;  // Pas de flags spécifiques
  sigemptyset(&C.sa_mask); 

  if (sigaction(SIGINT, &C, NULL) == -1) 
  {
    perror("(SERVEUR) Erreur lors de l'armement du signal SIGINT");
    exit(1);
  }

  struct sigaction A;
  A.sa_handler = HandlerSIGCHLD;
  sigemptyset(&A.sa_mask);
  A.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD,&A,NULL) == -1)
  {
    perror("Erreur de sigaction");
    exit(1);
  }

  // Creation des ressources
  // Creation de la file de message
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  //creation de la memoire partagée
  fprintf(stderr,"(SERVEUR %d) Creation de la memoire partagée\n",getpid());
  idShm = shmget(CLE, 52 * sizeof(char), IPC_CREAT | 0666);
  if (idShm == -1) 
  {
    perror("Erreur lors de la création de la mémoire partagée");
    exit(1);
  }

  if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
    perror("Erreur lors de l'attachement à la memoire partagée");
    exit(1);
  }

  // TO BE CONTINUED

  // Creation du pipe
  // TO DO
  fprintf(stderr,"(SERVEUR %d) Creation du pipe\n",getpid());
  if (pipe(fdPipe) == -1)
  {
      perror("(SERVEUR) Erreur lors de la création du pipe");
      exit(1);
  }


  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = getpid();
  tab->pidPublicite = 0;

  // Creation du processus Publicite (étape 2)
  // TO DO
  pid_t Publicite = fork();  // Crée un processus enfant

  if (Publicite == -1)
  {
      perror("(SERVEUR) Erreur lors de la création du processus Publicité");
      exit(1);
  }
  else if(Publicite==0)
  {
      execl("./Publicite", "Publicite", NULL); 
      // Si execl échoue
      perror("Erreur lors du lancement de publicite");
      exit(1);
  }
  tab->pidPublicite = Publicite;
  afficheTab();
  // Creation du processus AccesBD (étape 4)
  // TO DO
  fprintf(stderr, "(SERVEUR %d) Création du processus AccesBD\n", getpid());
  pid_t AccesBD = fork();
  if (AccesBD == -1)
  {
      perror("(SERVEUR) Erreur lors de la création du processus AccesBD");
      exit(1);
  }
  else if (AccesBD == 0) 
  {
      close(fdPipe[1]);  // Fermer l'écriture du pipe dans AccesBD
      char fdLecture[10];
      sprintf(fdLecture, "%d", fdPipe[0]);
      execl("./AccesBD", "AccesBD", fdLecture, NULL);  // Passage du descripteur de lecture du pipe
      perror("Erreur lors du lancement d'AccesBD");
      exit(1);
  }
  tab->pidAccesBD = AccesBD;
  close(fdPipe[0]); 

  MESSAGE m;
  MESSAGE reponse;
  sigsetjmp(contexte, 1);

  while(1)
  {
  	//fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  // TO DO
                      {
                        fprintf(stderr, "(SERVEUR %d) Requete CONNECT reçue de %d\n", getpid(), m.expediteur);
                        for (int i = 0; i < 6; i++) 
                        {
                          if (tab->connexions[i].pidFenetre == 0) 
                          {
                            tab->connexions[i].pidFenetre = m.expediteur;
                            strcpy(tab->connexions[i].nom, " ");
                            tab->connexions[i].pidCaddie = 0;
                            printf("(SERVEUR) Nouveau client connecté : %s (PID : %d)\n", m.data2, m.expediteur);
                            break;
                          }
                        }
                        break;
                      }
                     

      case DECONNECT : // TO DO
                      {
                        fprintf(stderr, "(SERVEUR %d) Requete DECONNECT reçue de %d\n", getpid(), m.expediteur);
                        for (int i = 0; i < 6; i++) 
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur) 
                          {
                            printf("(SERVEUR) Déconnexion de la fenêtre %d\n", m.expediteur);
                            tab->connexions[i].pidFenetre = 0;
                            break;
                          }
                        }
                        break;
                      }
                      
    case LOGIN :    // TO DO
                    {
                        fprintf(stderr, "(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n", getpid(), m.expediteur, m.data1, m.data2, m.data3);
                        fprintf(stderr, "(SERVEUR %d) Valeurs reçues : data1=%d, data2=%s, data3=%s\n",getpid(), m.data1, m.data2, m.data3);
                        int position = estPresent(m.data2);  // Vérifier si l'utilisateur existe
                        MESSAGE reponse;
                        memset(&reponse, 0, sizeof(MESSAGE));
                        reponse.type = m.expediteur;
                        reponse.requete = LOGIN;
                        if (position>0)  // Si l'utilisateur existe
                        {
                            if (m.data1 == 1)  // Si la case "Nouveau client" est cochée
                            {
                                // L'utilisateur existe déjà, mais a coché "Nouveau client"
                                reponse.data1 = 0;
                                strcpy(reponse.data4, "Cet utilisateur existe déjà\n");
                            }
                            else  // Si la case "Nouveau client" n'est pas cochée
                            {
                                // Vérifier le mot de passe de l'utilisateur
                                if (verifieMotDePasse(position, m.data3))  // Si mot de passe correct
                                {
                                    printf("(SERVEUR) Login réussi pour %s\n", m.data2);
                                    ajouterUtilisateurDansTable(m);  // Ajouter à la table des connexions

                                    reponse.data1 = 1;
                                    strcpy(reponse.data4, "Connexion réussie");
                                }
                                else  // Mot de passe incorrect
                                {
                                    reponse.data1 = 0;
                                    strcpy(reponse.data4, "Mot de passe incorrect");
                                }
                            }
                        }
                        else  // Si l'utilisateur n'existe pas (nouveau client)
                        {
                            if (m.data1 == 1)  // Si la case "Nouveau client" est cochée
                            {
                                // Créer un nouvel utilisateur
                                printf("(SERVEUR) Création d'un nouvel utilisateur : %s\n", m.data2);
                                ajouteUtilisateur(m.data2, m.data3);  // Fonction pour ajouter un nouvel utilisateur
                                ajouterUtilisateurDansTable(m);  // Ajouter à la table des connexions

                                reponse.data1 = 1;
                                strcpy(reponse.data4, "Bienvenue, nouvel utilisateur!");
                            }
                            else  // Si l'utilisateur n'existe pas et que la case "Nouveau client" n'est pas cochée
                            {
                                // L'utilisateur est inexistant, mais la case n'est pas cochée, renvoyer un message d'erreur
                                reponse.data1 = 0;
                                strcpy(reponse.data4, "Utilisateur inexistant. Si vous êtes nouveau, cochez la case.");
                            }
                        }
                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
                        {
                            perror("(SERVEUR) Erreur lors de l'envoi de la réponse LOGIN");
                            exit(1);
                        }
                        
                        if (kill(m.expediteur, SIGUSR1) == -1) 
                        {
                            perror("(SERVEUR) Erreur lors de l'envoi du signal SIGUSR1");
                            exit(1);
                        }

                        for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête CONSULT");
                            }
                          }
                          break;
                        }
                      }

                        break;
                    }


      case LOGOUT :   // TO DO
                    {
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      // Recherche de l'utilisateur dans la table des connexions
                      for (int i = 0; i < 6; i++) 
                      {
                        if (tab->connexions[i].pidFenetre == m.expediteur) 
                        {
                            m.type = tab->connexions[i].pidCaddie;
                           
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête LOGOUT");
                            }

                            // Réinitialiser les informations de connexion, mais garder le PID de la fenêtre
                            printf("(SERVEUR) LOGOUT de %s (PID: %d)\n", tab->connexions[i].nom, m.expediteur);
                            
                            //tab->connexions[i].pidCaddie = 0;  
                            strcpy(tab->connexions[i].nom, "");  
                            break;
                        }
                      }
                      break;
                    }
                      

      case UPDATE_PUB :  // TO DO
                      {
                        //fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                        for (int i = 0; i < 6; i++) 
                        {
                          if (tab->connexions[i].pidFenetre > 0) 
                          {
                          // Envoyer le signal SIGUSR2 pour chaque client connecté
                            if (kill(tab->connexions[i].pidFenetre, SIGUSR2) == -1) 
                            {
                                perror("Erreur lors de l'envoi du signal SIGUSR2");
                                exit(1);
                            }
                          }
                        }
                      break;
                      }

      case CONSULT :  // TO DO
                    {
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          int pidCaddie = tab->connexions[i].pidCaddie;

                          if(pidCaddie > 0)
                          {
                            m.type = pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête CONSULT");
                            }
                          }
                          break;
                        }
                      }

                      break;
                    }

      case ACHAT :    // TO DO
                  {
                    fprintf(stderr,"(SERVEUR %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                    for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête ACHAT");
                            }
                          }
                          break;
                        }
                      }
                      break;
                  }
                      

      case CADDIE :   // TO DO
                    {
                    fprintf(stderr,"(SERVEUR %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                    for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête CONSULT");
                            }
                          }
                          break;
                        }
                      }
                      break;
                  }

      case CANCEL :   // TO DO
                      {
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);

                      for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête ACHAT");
                            }
                          }
                          break;
                        }
                      }
                      break;
                      }

      case CANCEL_ALL : // TO DO
                      {
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);
                      for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête ACHAT");
                            }
                          }
                          break;
                        }
                      }
                
                      }

      case PAYER : // TO DO
                      {

                      
                      fprintf(stderr,"(SERVEUR %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                      for(int i = 0;i < 6; i++)
                      {
                        if(tab->connexions[i].pidFenetre == m.expediteur)
                        {
                          if(tab->connexions[i].pidCaddie > 0)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1) 
                            {
                              perror("(SERVEUR) Erreur lors de l'envoi de la requête ACHAT");
                            }
                          }
                          break;
                        }
                      }
                      break;
                      }

      case NEW_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    if(m.requete!= UPDATE_PUB)
    {
      afficheTab();
    }
    
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}

void ajouterUtilisateurDansTable(MESSAGE m) 
{
    bool ajout=false;
    for(int i=0;i<6;i++)
    {
      if ( tab->connexions[i].pidFenetre == m.expediteur) 
      {
        strcpy(tab->connexions[i].nom, m.data2);
        ajout=true;
        // Creation du processus Caddie (étape 2)
        // TO DO
        pid_t Caddie = fork();  // Crée un processus enfant

        if (Caddie == -1)
        {
            perror("(SERVEUR) Erreur lors de la création du processus Caddie");
            exit(1);
        }
        else if(Caddie==0)  // Si on est dans le processus enfant (Caddie)
        {
            printf("(SERVEUR) lancement du Processus Caddie PID = %d\n", getpid());
            close(fdPipe[0]);  // Fermer la lecture du pipe dans le Caddie
            char fdEcriture[10];
            sprintf(fdEcriture, "%d", fdPipe[1]);
            execl("./Caddie", "Caddie", fdEcriture, NULL);  // Passage du descripteur d'écriture du pipe
            perror("Erreur lors du lancement de Caddie");
            exit(1);
        }
        tab->connexions[i].pidCaddie = Caddie;
        break;
      } 

    }
    if(ajout==false) 
    {
      MESSAGE reponse;
      strcpy(reponse.data4, "Table des connexions pleine");
    }
}

void handleSIGINT(int sig) 
{

    fprintf(stderr, "(SERVEUR) Envoi de SIGINT au processus AccesBD PID=%d\n", tab->pidAccesBD);
    if (kill(tab->pidAccesBD, SIGINT) == -1)
    {
        perror("(SERVEUR) Erreur lors de l'envoi du signal SIGINT à AccesBD");
    }

    fprintf(stderr, "(SERVEUR) Envoi de SIGINT au processus Publicité PID=%d\n", tab->pidPublicite);
    if(kill(tab->pidPublicite,SIGINT)==-1)
    {
      perror("(SERVEUR) Erreur lors de la suppression du fils publicité\n");
    }
    //Terminer tous les processus Caddie
    fprintf(stderr, "(SERVEUR %d) Terminaison des processus Caddie.\n", getpid());
    for (int i = 0; i < 6; i++) 
    { 
        if (tab->connexions[i].pidCaddie > 0) 
        {
            fprintf(stderr, "(SERVEUR) Envoi de SIGINT au processus Caddie PID=%d\n", tab->connexions[i].pidCaddie);
            if (kill(tab->connexions[i].pidCaddie, SIGINT) == -1) 
            {
                perror("(SERVEUR) Erreur lors de l'envoi de SIGTERM");
            }
            tab->connexions[i].pidCaddie = 0; // Réinitialiser la table
        }
    }
    
  fprintf(stderr, "(SERVEUR %d) Fermeture du pipe\n", getpid());
  close(fdPipe[0]);
  close(fdPipe[1]);
    

    fprintf(stderr, "(SERVEUR %d) Signal SIGINT reçu, suppression de la file de messages...\n", getpid());
    if (msgctl(idQ, IPC_RMID, NULL) == -1) 
    {
      perror("(SERVEUR) Erreur lors de la suppression de la file de messages");
    } 
    else 
    {
        printf("(SERVEUR) File de messages supprimée avec succès\n");
    }

   

    if (shmdt(pShm) == -1) 
    {
      perror("Erreur lors du détachement de la mémoire partagée");
      exit(1);
    }

    fprintf(stderr, "(SERVEUR %d) Signal SIGINT reçu, suppression de la memoire partaée...\n", getpid());

    if (shmctl(idShm,IPC_RMID,NULL) ==-1)
    {
      perror("(SERVEUR) Erreur lors de la suppression de la file de messages");
      exit(1);
    }
    else
    {
      printf("(SERVEUR) memoire partagée supprimée avec succès\n");
    }
    
    exit(0);
}

void HandlerSIGCHLD(int sig)
{
    int id;
    int status;
    int resultat;

    // Attendre que n'importe quel enfant se termine
    while((id = wait(&status)) != -1) 
    {
        // Vérifier si l'enfant s'est terminé normalement
        if (WIFEXITED(status)) 
        {
            // Récupérer le code de sortie du processus
            resultat = WEXITSTATUS(status);

            for (int i = 0; i < 6; i++) 
            {
                if (tab->connexions[i].pidCaddie == id) 
                {
                    printf("(SERVEUR %d) suppression du fils zombi (PID: %d)\n", getpid(), id);
                    tab->connexions[i].pidCaddie = 0;
                    siglongjmp(contexte,sig);
                    
                }
            }
        } 
        else
        {
            fprintf(stderr, "(SERVEUR %d) Processus Caddie %d mal terminén", getpid(), id);
        }
    }
    
    

}



