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
MYSQL* connexion;

MESSAGE reponse;

char requete[200];
char newUser[20];
MYSQL_RES  *resultat;
MYSQL_ROW  Tuple;

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(ACCESBD %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(ACCESBD) Erreur de msgget");
    exit(1);
  }

  // Récupération descripteur lecture du pipe
  int fdRpipe = atoi(argv[1]);

  // Connexion à la base de donnée
  // TO DO
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion, "localhost", "Student", "PassStudent1_", "PourStudent", 0, 0, 0) == NULL) 
  {
      fprintf(stderr, "(ACCESBD) Erreur de connexion à la base de données...\n");
      exit(1);
  }
  else
  {
    printf("(ACCESBD) Connexion à la base de données réussie\n");
  }
  

  MESSAGE m;

  while(1)
  {
    // Lecture d'une requete sur le pipe
    // TO DO
    ssize_t nbLu = read(fdRpipe, &m, sizeof(MESSAGE));
    if (nbLu <0) 
    {
      perror("(ACCESBD) Erreur de lecture du pipe");
      break;
    }
    else if (nbLu == 0) 
    {
      fprintf(stderr, "(ACCESBD %d) Fin de l'écriture dans le pipe, terminaison...\n", getpid());
      break;
    }

    switch(m.requete)
    {
      case CONSULT :  // TO DO
                    {
                      fprintf(stderr,"(ACCESBD %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD
                      // Formulation de la requête SQL pour chercher l'article par son ID
                      sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d", m.data1);

                      if (mysql_query(connexion, requete)) 
                      {
                        fprintf(stderr, "(ACCESBD) Erreur de requête SQL : %s\n", mysql_error(connexion));
                        reponse.data1 = -1; // En cas d'erreur, indiquer que l'article n'a pas été trouvé
                        break;
                      }
                      resultat = mysql_store_result(connexion);
                      Tuple = mysql_fetch_row(resultat);

                      // Preparation de la reponse
                      if (Tuple != NULL) 
                      {
                        // Article trouvé, préparer la réponse
                        reponse.type = m.expediteur;   // On envoie la réponse au bon Caddie
                        reponse.requete = CONSULT;
                        reponse.data1 = atoi(Tuple[0]);  // ID
                        strcpy(reponse.data2, Tuple[1]); // Intitulé
                        reponse.data5 = atof(Tuple[2]);  // Prix
                        sprintf(reponse.data3, "%d", atoi(Tuple[3])); // Stock
                        strcpy(reponse.data4, Tuple[4]); // Image

                        printf("(ACCESBD %d) Article trouvé : %s\n", getpid(), Tuple[1]);
                      }
                      else 
                      {
                        // Article non trouvé
                        reponse.data1 = -1;
                        printf("(ACCESBD %d) Article non trouvé\n", getpid());
                      }


                      // Envoi de la reponse au bon caddie
                      if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
                      {
                        perror("(ACCESBD) Erreur lors de l'envoi de la réponse CONSULT");
                        exit(1);
                      }
                      mysql_free_result(resultat);

                      break;
                   }

      case ACHAT :    // TO DO
                    {
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD
                       int quantiteAchat = atoi(m.data2);
                       printf("achat: quantite achat =%d\n",quantiteAchat);
                      // Vérifier le stock disponible
                      sprintf(requete, "SELECT* FROM UNIX_FINAL WHERE id = %d", m.data1);
                      
                      if (mysql_query(connexion, requete))
                      {
                          fprintf(stderr, "(ACCESBD) Erreur de requête SQL : %s\n", mysql_error(connexion));
                          reponse.data1 = -1;  // Indiquer une erreur dans la requête
                          break;
                      }
                      
                      resultat = mysql_store_result(connexion);
                      Tuple = mysql_fetch_row(resultat);
                      int stockDisponible = atoi(Tuple[3]);
                      

                      if (stockDisponible>=quantiteAchat)  // Vérifier si le stock est suffisant
                      {
                          // Mettre à jour le stock
                          
                          int newStock = stockDisponible - quantiteAchat;
                          
                          sprintf(requete, "UPDATE UNIX_FINAL SET stock = %d WHERE id = %d",newStock,m.data1);
                          

                          if (mysql_query(connexion, requete))
                          {
                              fprintf(stderr, "(ACCESBD) Erreur de mise à jour de la base de données : %s\n", mysql_error(connexion));
                              reponse.data1 = -1;
                              break;
                          }
                          
                          

                          // Préparer la réponse
                          reponse.data1 = atoi(Tuple[0]);  // ID
                          reponse.type=m.expediteur;
                          
                          strcpy(reponse.data2, Tuple[1]); // Intitulé
                          
                          reponse.data5 = atof(Tuple[2]);  // Prix
                          //reponse.data2=m.data2;
                          
                          sprintf(reponse.data3,"%d",quantiteAchat);
                          
                          strcpy(reponse.data4, Tuple[4]);

                          reponse.requete=ACHAT;  
                          

                          // Réponse envoyée à Caddie
                          if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                              perror("(ACCESBD) Erreur lors de l'envoi de la réponse ACHAT");
                              exit(1);
                          }
                          

                          fprintf(stderr, "(ACCESBD %d) Achat réussi, stock mis à jour pour l'article ID = %d\n", getpid(), m.data1);
                      }
                      else  // Si le stock est insuffisant
                      {
                          reponse.data1 = m.data1;
                          reponse.type=m.expediteur;
                          sprintf(reponse.data3,"0");
                          reponse.requete=ACHAT;
                         

                          strcpy(reponse.data2, Tuple[1]); // Intitulé
                          
                          reponse.data5 = atof(Tuple[2]);  // Prix
                          
                          
                          strcpy(reponse.data4, Tuple[4]);
                         
                          if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                          {
                              perror("(ACCESBD) Erreur lors de l'envoi de la réponse de stock insuffisant");
                              exit(1);
                          }
                          fprintf(stderr, "(ACCESBD %d) Achat échoué, stock insuffisant pour l'article ID = %d\n", getpid(), m.data1);
                      }

                      mysql_free_result(resultat);
                      

                      // Finalisation et envoi de la reponse
                      break;
                    }

      case CANCEL :   // TO DO
                      {
                        fprintf(stderr,"(ACCESBD %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                        // Acces BD
                         // Vérification de l'article dans la base de données pour obtenir le stock actuel
                        char requete[200];
                        sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE id = %d", m.data1);

                        if (mysql_query(connexion, requete)) 
                        {
                            fprintf(stderr, "(ACCESBD) Erreur lors de la requête SQL : %s\n", mysql_error(connexion));
                            break;
                        }

                        resultat = mysql_store_result(connexion);
                        if (resultat == NULL) 
                        {
                            fprintf(stderr, "(ACCESBD) Erreur lors de la récupération des résultats de la requête\n");
                            break;
                        }

                        Tuple = mysql_fetch_row(resultat);
                        if (Tuple != NULL) 
                        {
                            // Le stock actuel de l'article
                            
                            int stockActuel = atoi(Tuple[3]);
                           
                            
                            // Réincrémentation du stock en fonction de la quantité supprimée
                            int nouveauStock = stockActuel + atoi(m.data2);
                           

                            // Mise à jour du stock dans la base de données
                            sprintf(requete, "UPDATE UNIX_FINAL SET stock = %d WHERE id = %d", nouveauStock, m.data1);

                            if (mysql_query(connexion, requete)) 
                            {
                                fprintf(stderr, "(ACCESBD) Erreur lors de la mise à jour du stock : %s\n", mysql_error(connexion));
                                break;
                            }

                            // Logique réussie, envoyer une réponse de confirmation
                            fprintf(stderr, "(ACCESBD) Stock réincrémenté avec succès pour l'article ID=%d. Nouveau stock = %d\n", m.data1, nouveauStock);
                        } 
                        else 
                        {
                            fprintf(stderr, "(ACCESBD) Article non trouvé pour l'ID %d\n", m.data1);
                        }

                        mysql_free_result(resultat);

                        // Envoyer une réponse au processus Caddie pour confirmer que le stock a été mis à jour
                        MESSAGE reponse;
                        memset(&reponse, 0, sizeof(MESSAGE));
                        reponse.type = m.expediteur;
                        reponse.expediteur=getpid();
                        reponse.requete = CANCEL;
                        reponse.data1 = m.data1;  // ID de l'article

                        // Envoi de la réponse
                        if (msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
                        {
                            perror("(ACCESBD) Erreur lors de l'envoi de la réponse CANCEL au Caddie");
                            break;
                        }

                        // Mise à jour du stock en BD
                        break;
                      }

    }
  }

  if (connexion != NULL) 
  {
    mysql_close(connexion);
    fprintf(stderr, "(ACCESBD %d) Connexion à la base de données fermée\n", getpid());
  }
 
  close(fdRpipe);
  fprintf(stderr, "(ACCESBD %d) Descripteur de pipe fermé\n", getpid());


}

