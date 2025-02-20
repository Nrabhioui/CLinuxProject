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
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
char *pShm;
void handlerSIGUSR1(int sig);
void handlerSIGINT(int sig);
int fd;

int main()
{
  // Armement des signaux
  // TO DO
  struct sigaction C;
  C.sa_handler=handlerSIGUSR1;
  C.sa_flags=0;
  sigemptyset(&C.sa_mask);
  if (sigaction(SIGUSR1, &C, NULL) == -1) 
  {
    perror("Erreur lors de l'armement du signal SIGUSR2");
    exit(1);
  }

  struct sigaction D;
  D.sa_handler=handlerSIGINT;
  D.sa_flags=0;
  sigemptyset(&D.sa_mask);
  if (sigaction(SIGINT, &D, NULL) == -1) 
  {
    perror("Erreur lors de l'armement du signal SIGINT");
    exit(1);
  }

  // Masquage des signaux
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask,SIGUSR1);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1) 
  {
    perror("(CLIENT) Erreur lors de la récupération de la file de messages");
    exit(1);
  }

  fprintf(stderr,"(PUBLICITE %d) Recuperation de la memoire partagée\n",getpid());
  if ((idShm = shmget(CLE,0,0)) ==-1)
  {
    perror("Erreur lors de la recuperation de la memoire partagée");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée

  // Attachement à la mémoire partagée
  if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
    perror("Erreur lors de l'attachement à la memoire partagée");
    exit(1);
  }

  // Mise en place de la publicité en mémoire partagée
  char pub[51];
  strcpy(pub,"Bienvenue sur le site du Maraicher en ligne !");

  for (int i=0 ; i<=50 ; i++) pShm[i] = ' ';
  pShm[51] = '\0';
  int indDebut = 25 - strlen(pub)/2;
  for (int i=0 ; i<strlen(pub) ; i++) pShm[indDebut + i] = pub[i];

  while(1)
  {  
    // Envoi d'une requete UPDATE_PUB au serveur
    MESSAGE pub_message;
    pub_message.expediteur=getpid();
    pub_message.type = 1;  // Type de message pour le serveur
    pub_message.requete = UPDATE_PUB;  // Requête de mise à jour de la publicité
    strcpy(pub_message.data4, pub);  // Copier le texte de la publicité dans le message

    
    if (msgsnd(idQ, &pub_message, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("Erreur lors de l'envoi de la publicité");
        exit(1);
    }
  
    

    sleep(1); 

    // Decallage vers la gauche

    char firstChar = pShm[0];  // Sauvegarder le premier caractère
    for (int i = 0; i < 50; i++) 
    {
      pShm[i] = pShm[i + 1];  // Décaler tous les caractères à gauche
    }
    pShm[50] = firstChar;  // Remettre le premier caractère à la fin
    pShm[51] = '\0'; 
    }
    
}

void handlerSIGUSR1(int sig)
{
  fprintf(stderr,"(PUBLICITE %d) Nouvelle publicite !\n",getpid());

  // Lecture message NEW_PUB

  // Mise en place de la publicité en mémoire partagée
}
void handlerSIGINT(int sig)
{
  fprintf(stderr, "(PUBLICITE %d) Reçu SIGINT, arrêt du processus Publicité\n", getpid());
  if(shmdt(pShm) == -1) 
  {
      perror("Erreur lors du détachement de la mémoire partagée");
      exit(1);
  }

}

