#include "FichierUtilisateur.h"
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int estPresent(const char* nom)
{
  // TO DO
  int pos = 1;
  UTILISATEUR utilisateur;
  int fichier =open(FICHIER_UTILISATEURS,O_RDONLY);
 
  if (fichier == -1)
  {
    return -1;
  }else
  {
    while(read(fichier,&utilisateur, sizeof(UTILISATEUR)) > 0)
    {
      if(strcmp(utilisateur.nom,nom) == 0)
      {
        close(fichier);
        return pos;
      }
      pos++;
 
    }
    return 0;
 
  }
}


////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  int i,H=0;

  for(i=0;motDePasse[i]!='\0';i++)
  {
    H+= motDePasse[i]*(i+1);
  }

  return H%97;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  int fd;
  fd = open(FICHIER_UTILISATEURS, O_WRONLY | O_APPEND | O_CREAT,0644);
  
  if (fd == -1)
  {
    perror("Erreur lors de l'ouverture ou de la création du fichier");
    return ;
  }

  UTILISATEUR user;
  strncpy(user.nom, nom, sizeof(user.nom) - 1);
  user.nom[sizeof(user.nom) - 1] = '\0';

  user.hash = hash(motDePasse); // bon,appelons la fonction hash pour calculer le hash du mot de passe

  int bW = write(fd, &user, sizeof(UTILISATEUR));
  if (bW != sizeof(UTILISATEUR))
  {
    perror("Erreur lors de l'écriture dans le fichier");
  }

  close(fd);

}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  int fd;
  fd = open(FICHIER_UTILISATEURS, O_RDONLY);

  if (fd == -1)
  {
    perror("Erreur lors de l'ouverture du fichier");
    return -1; 
  }

  UTILISATEUR user;

  // Déplacement à la position de l'utilisateur (pos-1 car lseek commence à 0)
  off_t offset = (pos - 1) * sizeof(UTILISATEUR);
  if (lseek(fd, offset, SEEK_SET) == -1)
  {
    perror("Erreur lors du déplacement dans le fichier");
    close(fd);
    return -1; 
  }

  if (read(fd, &user, sizeof(UTILISATEUR)) != sizeof(UTILISATEUR))
  {
    perror("Erreur lors de la lecture de l'utilisateur");
    close(fd);
    return -1;
  }

  int hashMotDePasse = hash(motDePasse);

  if (user.hash == hashMotDePasse)
  {
    close(fd);
    return 1; 
  }
  else
  {
    close(fd);
    return 0; 
  }
}


////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  int fd;

  fd = open(FICHIER_UTILISATEURS, O_RDONLY);

  if (fd == -1)
  {
    printf("Erreur lors de l'ouverture du fichier");
    return -1;
  }

  int count = 0;
  int bR;

  while ((bR = read(fd, &vecteur[count], sizeof(UTILISATEUR))) == sizeof(UTILISATEUR))
  {
    count++;
  }

  if (bR == -1)
  {
    printf("Erreur lors de la lecture du fichier");
    close(fd);
    return -1;
  }

  close(fd);

  return count; 
}

