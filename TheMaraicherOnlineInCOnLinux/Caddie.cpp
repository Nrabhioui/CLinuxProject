#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mysql.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ;

ARTICLE articles[10];
int nbArticles = 0;
 int pidClient;

int fdWpipe;


void handlerSIGALRM(int sig);

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  /*sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);*/

  // Armement des signaux
  // TO DO
  struct sigaction sa;
  sa.sa_handler = handlerSIGALRM;
  sa.sa_flags = IPC_NOWAIT;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGALRM, &sa, NULL) == -1) 
  {
      perror("(CADDIE) Erreur lors de l'armement du signal SIGALRM");
      exit(1);
  }


  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CADDIE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(CADDIE) Erreur de msgget");
    exit(1);
  }

  MESSAGE m;
  

  // Récupération descripteur écriture du pipe
  fdWpipe = atoi(argv[1]);

  while(1)
  {
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
      perror("(CADDIE) Erreur de msgrcv");
      exit(1);
    }


    switch(m.requete)
    {
      case LOGIN :    // TO DO
                   {
                     fprintf(stderr,"(CADDIE %d) Requete LOGIN reçue de %d\n",getpid(),m.expediteur);
                     // Mémoriser le pid du client
                      pidClient = m.expediteur;
                      break;
                   }   

      case LOGOUT :   // TO DO
                    {
                      fprintf(stderr,"(CADDIE %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);

                      exit(0); 
                      break;
                    } 

      case CONSULT :  // TO DO
                    {
                      fprintf(stderr,"(CADDIE %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      pidClient = m.expediteur;
                      // Modifier le pid de l'expéditeur du message et y mettre le propre pid du Caddie
                      m.expediteur = getpid(); 

                      // Transmettre la requête au processus AccesBD via le pipe
                      if (write(fdWpipe, &m, sizeof(MESSAGE)) == -1)
                      {
                          perror("(CADDIE) Erreur lors de l'écriture dans le pipe vers AccesBD");
                          exit(1);
                      }
                      fprintf(stderr, "(CADDIE %d) Ecriture dans le pipe de r  CONSULT envoyée à AccesBD pour l'article ID = %d\n", getpid(), m.data1);
                      
                      MESSAGE reponse;
                      // Attente de la réponse du processus AccesBD
                      if (msgrcv(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
                      {
                          perror("(CADDIE) Erreur lors de la réception de la réponse d'AccesBD");
                          exit(1);
                      }
                      fprintf(stderr, "(CADDIE %d) Réponse reçue : Requête=%d, ID=%d, Intitulé=%s, Prix=%.2f, Stock=%s\n",
                                      getpid(), reponse.requete, reponse.data1, reponse.data2, reponse.data5, reponse.data3);

                      // Transmettre la réponse au client
                      if (reponse.data1 != -1) // Si l'article est trouvé
                      {
                          reponse.type=pidClient;
                          if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                              perror("(CADDIE) Erreur lors de l'envoi de la réponse CONSULT au client");
                              exit(1);
                          }
                          fprintf(stderr, "(CADDIE %d) Réponse CONSULT envoyée au client %d\n", getpid(),pidClient );

                          if (kill(pidClient, SIGUSR1) == -1) 
                          {
                            perror("(CADDIE) Erreur lors de l'envoi du signal au client");
                            exit(1);
                          }
                      }
                      else
                      {
                          fprintf(stderr, "(CADDIE %d) L'article ID = %d non trouvé, pas de réponse envoyée.\n", getpid(), m.data1);
                      }
                      break;
                    }

      case ACHAT :    // TO DO
      {
                      fprintf(stderr,"(CADDIE %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      pidClient = m.expediteur;
                     
                      m.expediteur = getpid();

                      // on transfert la requete à AccesBD
                      if (write(fdWpipe, &m, sizeof(MESSAGE)) == -1)
                      {
                          perror("(CADDIE) Erreur lors de l'écriture dans le pipe vers AccesBD");
                          exit(1);
                      }
                      fprintf(stderr, "(CADDIE %d) Ecriture dans le pipe de la requete ACHAT à envoyée à AccesBD pour l'article ID = %d\n", getpid(), m.data1);
                      
                      // on attend la réponse venant de AccesBD
                      MESSAGE reponse;
                      if (msgrcv(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1)
                      {
                          perror("(CADDIE) Erreur lors de la réception de la réponse d'AccesBD");
                          exit(1);
                      }

                     
                      // Envoi de la reponse au client
                      if (atoi(reponse.data3)> 0)  // Achat réussi
                      {
                          // Ajouter l'article dans le panier
                          if(nbArticles<10)
                          {

                          
                          articles[nbArticles].id = reponse.data1;  // Id de l'article
                          strcpy(articles[nbArticles].intitule, reponse.data2);  // Intitulé
                          articles[nbArticles].prix = reponse.data5;  // Prix
                          articles[nbArticles].stock = atoi(reponse.data3);  // Quantité
                          nbArticles++; 
                          }
                          else
                          {
                            fprintf(stderr, "(CADDIE %d) Panier plein\n", getpid());

                          } 
                      }
                      else
                      {
                        fprintf(stderr, "(CADDIE %d) Achat échoué : Stock insuffisant pour l'article ID=%d\n", getpid(), m.data1);

                      }
                      reponse.type=pidClient;

                      // Passer au client la réponse d'achat
                      if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                      {
                          perror("Erreur lors de l'envoi de la réponse d'achat");
                          exit(1);
                      }

                      if (kill(pidClient, SIGUSR1) == -1) 
                      {
                        perror("(CADDIE) Erreur lors de l'envoi du signal au client");
                        exit(1);
                      }
                      fprintf(stderr, "(CADDIE %d) Réponse ACHAT envoyée au client %d\n", getpid(), pidClient);
                    
                      

                      

                      break;
      }

      case CADDIE :   // TO DO
                    {
                      fprintf(stderr,"(CADDIE %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      fprintf(stderr, "(CADDIE %d) nbArticles = %d\n", getpid(), nbArticles);
                       // Envoi des articles un par un au client
                      int i = 0;
                      while( i < nbArticles)
                      {
                          MESSAGE m_article;
                         
                          m_article.type =m.expediteur;  // Type de message pour le client
                          m_article.expediteur=getpid();
                          m_article.requete = CADDIE;
                          m_article.data1 = articles[i].id;  // Id de l'article
                          strcpy(m_article.data2, articles[i].intitule);  // Intitulé de l'article
                          m_article.data5 = articles[i].prix;  // Prix de l'article
                          sprintf(m_article.data3, "%d", articles[i].stock);  // Quantité de l'article
                          strcpy(m_article.data4, articles[i].image);  // Image de l'article

                          if (msgsnd(idQ, &m_article, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                              perror("Erreur lors de l'envoi de l'article au client");
                              exit(1);
                          }

                          // Envoi du signal pour informer le client
                          if (kill(m.expediteur, SIGUSR1) == -1)
                          {
                              perror("Erreur lors de l'envoi du signal SIGUSR1");
                              exit(1);
                          }
                           fprintf(stderr, "(CADDIE %d) Article envoyé au client %d: ID=%d, %s, Quantité=%d\n", 
                                             getpid(),m.expediteur, articles[i].id, articles[i].intitule, articles[i].stock);
                          i++;
                      }

                      break;
                    }

      case CANCEL :   // TO DO
                      {
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL reçue de %d et indice = %d\n",getpid(),m.expediteur,m.data1);

                      // on transmet la requete à AccesBD
                      
                      if (m.data1 < 0 || m.data1 >= nbArticles) 
                      {
                        fprintf(stderr, "(CADDIE %d) Indice invalide : %d\n", getpid(), m.data1);
                        break;
                      }
                     

                      // Envoi à AccesBD pour mettre à jour la base de données
                      MESSAGE cancelReq;
                      memset(&cancelReq, 0, sizeof(MESSAGE));

                      
                      
                      cancelReq.requete = CANCEL;
                      cancelReq.expediteur=getpid();
                      cancelReq.data1 = articles[m.data1].id;  // ID de l'article
                      
                      sprintf(cancelReq.data2,"%d",articles[m.data1].stock);  // Quantité

                      // Envoyer la requête au processus AccesBD pour mettre à jour le stock
                      if (write(fdWpipe, &cancelReq, sizeof(MESSAGE)) == -1) 
                      {
                          perror("(CADDIE) Erreur lors de l'écriture dans le pipe vers AccesBD");
                          exit(1);
                      }

                      // Suppression de l'aricle du panier

                      for(int i=0;i<nbArticles -1;i++)
                      {
                        articles[i]=articles[i+1];
                      }
                       nbArticles--;  


                      
                      break;
                      }

      case CANCEL_ALL : // TO DO
                      {
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);

                      // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier
                      for (int i = 0; i < nbArticles; i++) 
                      {
                          MESSAGE cancelReq;
                          memset(&cancelReq, 0, sizeof(MESSAGE));

                          cancelReq.requete = CANCEL;
                          cancelReq.expediteur=getpid();
                          cancelReq.data1 = articles[i].id;  // ID de l'article
                          sprintf(cancelReq.data2, "%d", articles[i].stock);  // Quantité à supprimer

                          // Envoi de la requête au processus AccesBD
                          if (write(fdWpipe, &cancelReq, sizeof(MESSAGE)) == -1) 
                          {
                              perror("(CADDIE) Erreur lors de l'écriture dans le pipe vers AccesBD");
                              exit(1);
                          }

                          fprintf(stderr, "(CADDIE %d) Envoi de CANCEL pour l'article ID=%d\n", getpid(), articles[i].id);
                      }

    
                      nbArticles = 0;  // Vider le panier
                      break;
                      }

      case PAYER :    // TO DO
                  {
                    fprintf(stderr,"(CADDIE %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                    nbArticles = 0;  // Réinitialisation du nombre d'articles dans le panier

                    
                    MESSAGE reponse;
                    memset(&reponse, 0, sizeof(MESSAGE));
                    reponse.type = m.expediteur;
                    reponse.requete = PAYER;
                    reponse.data1 = 1;

                    //  Envoi de la réponse de paiement au client
                    if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
                    {
                        perror("(CADDIE) Erreur lors de l'envoi de la réponse PAYER");
                        exit(1);
                    }

                    fprintf(stderr, "(CADDIE %d) Panier vidé et paiement effectué avec succès\n", getpid());
                    break;
                                    
                  }
    }
  }
}

  // Envoi du signal TIME_OUT au client si le client existe toujours
      MESSAGE timeoutMsg;
      memset(&timeoutMsg, 0, sizeof(MESSAGE));
      timeoutMsg.requete = TIME_OUT;
      timeoutMsg.type = pidClient;
      timeoutMsg.expediteur=getpid();

      if (msgsnd(idQ, &timeoutMsg, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
      {
          perror("(CADDIE) Erreur lors de l'envoi de la requête TIME_OUT au client");
      }
      
      if (kill(pidClient, SIGUSR1) == -1)
      {
          perror("Erreur lors de l'envoi du signal SIGUSR1");
          exit(1);
      }
      printf("%d",pidClient);
      exit(0);
}