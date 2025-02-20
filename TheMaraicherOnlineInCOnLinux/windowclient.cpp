#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include <string>
using namespace std;

#include "protocole.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>

extern WindowClient *w;

int idQ, idShm;
bool logged;
char* pShm;
ARTICLE articleEnCours;
float totalCaddie = 0.0;

void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);

#define REPERTOIRE_IMAGES "images/"

WindowClient::WindowClient(QWidget *parent) : QMainWindow(parent), ui(new Ui::WindowClient)
{
    ui->setupUi(this);

    // Configuration de la table du panier (ne pas modifer)
    ui->tableWidgetPanier->setColumnCount(3);
    ui->tableWidgetPanier->setRowCount(0);
    QStringList labelsTablePanier;
    labelsTablePanier << "Article" << "Prix à l'unité" << "Quantité";
    ui->tableWidgetPanier->setHorizontalHeaderLabels(labelsTablePanier);
    ui->tableWidgetPanier->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetPanier->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetPanier->horizontalHeader()->setVisible(true);
    ui->tableWidgetPanier->horizontalHeader()->setDefaultSectionSize(160);
    ui->tableWidgetPanier->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetPanier->verticalHeader()->setVisible(false);
    ui->tableWidgetPanier->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
    // TO DO
    if ((idQ = msgget(CLE,0)) == -1) 
    {
      perror("(CLIENT) Erreur lors de la récupération de la file de messages");
      exit(1);
    }
    printf("(CLIENT) File de messages récupérée, idQ = %d\n", idQ);

    //Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    // TO DO
    if ((idShm = shmget(CLE,0,0)) ==-1)
    {
      perror("Erreur lors de la recuperation de la memoire partagée");
      exit(1);
    }
    printf("(CLIENT) File de messages récupérée, idShm = %d\n", idShm);


    // Attachement à la mémoire partagée
    // TO DO
    fprintf(stderr,"(CLIENT %d) attachement à la mémoire partagée\n",getpid());
    if ((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
    {
      perror("Erreur lors de l'attachement à la memoire partagée");
      exit(1);
    }

    // Armement des signaux
    // TO DO
    fprintf(stderr,"(CLIENT %d) Armement du siganl SIGUSR1\n",getpid());
    struct sigaction C;
    C.sa_handler=handlerSIGUSR1;
    C.sa_flags=0;
    sigemptyset(&C.sa_mask);
    if (sigaction(SIGUSR1, &C, NULL) == -1) 
    {
      perror("Erreur lors de l'armement du signal SIGUSR2");
      exit(1);
    }

    fprintf(stderr,"(CLIENT %d) Armement du siganl SIGUSR2\n",getpid());
    struct sigaction B;
    B.sa_handler = handlerSIGUSR2;
    B.sa_flags = 0;
    sigemptyset(&B.sa_mask);
    if (sigaction(SIGUSR2, &B, NULL) == -1) 
    {
      perror("Erreur lors de l'armement du signal SIGUSR2");
      exit(1);
    }


    // Envoi d'une requete de connexion au serveur
    // TO DO
    fprintf(stderr, "(CLIENT %d) Envoi de la requête CONNECT au serveur\n", getpid());
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  // Type de message pour le serveur
    m.requete = CONNECT;  // Requête de connexion
    m.expediteur = getpid();  // Utilise le PID du client comme expéditeur

    // Envoi du message de connexion au serveur
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
      perror("(CLIENT)  Erreur lors de l'envoi du message");
      exit(1);
    }


    // Exemples à supprimer
    /*setPublicite("Promotions sur les concombres !!!");
    setArticle("pommes",5.53,18,"pommes.jpg");
    ajouteArticleTablePanier("cerises",8.96,2);*/
}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setImage(const char* image)
{
  // Met à jour l'image
  char cheminComplet[80];
  sprintf(cheminComplet,"%s%s",REPERTOIRE_IMAGES,image);
  QLabel* label = new QLabel();
  label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  label->setScaledContents(true);
  QPixmap *pixmap_img = new QPixmap(cheminComplet);
  label->setPixmap(*pixmap_img);
  label->resize(label->pixmap()->size());
  ui->scrollArea->setWidget(label);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauClientChecked()
{
  if (ui->checkBoxNouveauClient->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setArticle(const char* intitule,float prix,int stock,const char* image)
{
  ui->lineEditArticle->setText(intitule);
  if (prix >= 0.0)
  {
    char Prix[20];
    sprintf(Prix,"%.2f",prix);
    ui->lineEditPrixUnitaire->setText(Prix);
  }
  else ui->lineEditPrixUnitaire->clear();
  if (stock >= 0)
  {
    char Stock[20];
    sprintf(Stock,"%d",stock);
    ui->lineEditStock->setText(Stock);
  }
  else ui->lineEditStock->clear();
  setImage(image);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getQuantite()
{
  return ui->spinBoxQuantite->value();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTotal(float total)
{
  if (total >= 0.0)
  {
    char Total[20];
    sprintf(Total,"%.2f",total);
    ui->lineEditTotal->setText(Total);
  }
  else ui->lineEditTotal->clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveauClient->setEnabled(false);

  ui->spinBoxQuantite->setEnabled(true);
  ui->pushButtonPrecedent->setEnabled(true);
  ui->pushButtonSuivant->setEnabled(true);
  ui->pushButtonAcheter->setEnabled(true);
  ui->pushButtonSupprimer->setEnabled(true);
  ui->pushButtonViderPanier->setEnabled(true);
  ui->pushButtonPayer->setEnabled(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->checkBoxNouveauClient->setEnabled(true);

  ui->spinBoxQuantite->setEnabled(false);
  ui->pushButtonPrecedent->setEnabled(false);
  ui->pushButtonSuivant->setEnabled(false);
  ui->pushButtonAcheter->setEnabled(false);
  ui->pushButtonSupprimer->setEnabled(false);
  ui->pushButtonViderPanier->setEnabled(false);
  ui->pushButtonPayer->setEnabled(false);

  setNom("");
  setMotDePasse("");
  ui->checkBoxNouveauClient->setCheckState(Qt::CheckState::Unchecked);

  setArticle("",-1.0,-1,"");

  w->videTablePanier();
  totalCaddie = 0.0;
  w->setTotal(-1.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du panier (ne pas modifier) /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteArticleTablePanier(const char* article,float prix,int quantite)
{
    char Prix[20],Quantite[20];

    sprintf(Prix,"%.2f",prix);
    sprintf(Quantite,"%d",quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetPanier->rowCount();
    nbLignes++;
    ui->tableWidgetPanier->setRowCount(nbLignes);
    ui->tableWidgetPanier->setRowHeight(nbLignes-1,10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetPanier->setItem(nbLignes-1,0,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetPanier->setItem(nbLignes-1,1,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetPanier->setItem(nbLignes-1,2,item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::videTablePanier()
{
    ui->tableWidgetPanier->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetPanier->selectionModel()->selectedRows();
    if (liste.size() == 0) return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue (ne pas modifier ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
  // TO DO (étape 1)
  // Envoi d'une requete DECONNECT au serveur

    if (totalCaddie > 0) 
    {
        // Si le panier n'est pas vide, envoyer une requête CANCEL_ALL
        MESSAGE cancelAllReq;
        memset(&cancelAllReq, 0, sizeof(MESSAGE));
        cancelAllReq.type = 1;  // Type de message pour le serveur
        cancelAllReq.requete = CANCEL_ALL;
        cancelAllReq.expediteur = getpid();  // Le PID du client

        if (msgsnd(idQ, &cancelAllReq, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
        {
            perror("(CLIENT) Erreur lors de l'envoi de la requête CANCEL_ALL");
            exit(1);
        }

        // Vider le panier et réinitialiser le total
        w->videTablePanier();
        totalCaddie = 0.0;
        w->setTotal(-1.0);
    }

  if (logged==true) 
  {
    // envoi d'un logout si logged
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type =1;  // Envoi du message au serveur
    m.requete = LOGOUT;
    m.expediteur = getpid();  // Le PID du client
    fprintf(stderr,"(Client %d) envois d'une Requete LOGOUT de %d\n",getpid(),m.expediteur);
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête LOGOUT");
        exit(1);
    }
    else
    {
      fprintf(stderr, "(CLIENT %d) Requête LOGOUT envoyée avec succès\n", getpid());
    }
  }
  
  // Envoi d'une requete de deconnexion au serveur
  MESSAGE m;
  memset(&m, 0, sizeof(MESSAGE));
  m.type = 1;  // Type de message pour le serveur
  m.requete = DECONNECT;  // Requête de déconnexion
  m.expediteur = getpid();  // Le PID du client

  // Envoi de la requête DECONNECT pour supprimer la fenêtre du serveur
  fprintf(stderr,"(Client %d) envois d'une Requete DECONNECT de %d\n",getpid(),m.expediteur);
  if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
  {
    perror("(CLIENT) Erreur lors de l'envoi de la requête DECONNECT");
    exit(1);
  }

  fprintf(stderr, "detachement de la memoire partagée\n");
  if (shmdt(pShm) == -1) 
  {
    perror("Erreur lors du détachement de la mémoire partagée");
    exit(1);
  }

  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    // Envoi d'une requete de login au serveur
    // TO DO
    // Créer le message à envoyer au serveur
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  // Type de message pour le serveur
    m.requete = LOGIN;  // Requête de connexion
    m.expediteur = getpid();  // Le PID du client
    m.data1=isNouveauClientChecked();  // //verifie s'il est nouveau ou pas
    strcpy(m.data2, getNom());  // Le nom d'utilisateur
    strcpy(m.data3, getMotDePasse()); //le mot de passe

    fprintf(stderr,"(Client %d) envois d'une Requete LOGIN au Serveur \n",getpid());
    fprintf(stderr, "(CLIENT %d) Contenu du message envoye: type=%ld, expediteur=%d, requete=%d, data1=%d, data2=%s, data3=%s\n",
        getpid(), m.type, m.expediteur, m.requete, m.data1, m.data2, m.data3);

    // Envoi du message au serveur
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
      perror("(CLIENT) Erreur lors de l'envoi de la requête LOGIN");
      exit(1);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogout_clicked()
{
    // Envoi d'une requete CANCEL_ALL au serveur (au cas où le panier n'est pas vide)
    // TO DO
    if (totalCaddie > 0) 
    {
        // Si le panier n'est pas vide, envoyer une requête CANCEL_ALL
        MESSAGE cancelAllReq;
        memset(&cancelAllReq, 0, sizeof(MESSAGE));
        cancelAllReq.type = 1;  // Type de message pour le serveur
        cancelAllReq.requete = CANCEL_ALL;
        cancelAllReq.expediteur = getpid();  // Le PID du client

        if (msgsnd(idQ, &cancelAllReq, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
        {
            perror("(CLIENT) Erreur lors de l'envoi de la requête CANCEL_ALL");
            exit(1);
        }

        // Vider le panier et réinitialiser le total
        w->videTablePanier();
        totalCaddie = 0.0;
        w->setTotal(-1.0);
    }


    // Envoi d'une requete de logout au serveur
    // TO DO
    // Envoi de la requête LOGOUT au serveur pour supprimer l'utilisateur de la table des connexions
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type =1;  // Envoi du message au serveur
    m.requete = LOGOUT;
    m.expediteur = getpid();  // Le PID du client

    fprintf(stderr,"(Client %d) envois d'une Requete LOGOUT au Serveur \n",getpid());

    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
      perror("(CLIENT) Erreur lors de l'envoi de la requête LOGOUT");
      exit(1);
    }

    // Mise à jour de l'interface utilisateur (désactivation des boutons)
    logoutOK();


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSuivant_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
    // Vérifie si l'article actuel est défini
    if (articleEnCours.id > 0)
    {
        
        int idSuivant = articleEnCours.id + 1;
        
        
        MESSAGE m;
        m.type = 1;  
        m.requete = CONSULT;  
        m.data1 = idSuivant;  
        m.expediteur = getpid();  

        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
        {
            perror("(CLIENT) Erreur lors de l'envoi de la requête CONSULT");
            exit(1);
        }
        fprintf(stderr, "Requete consult envoyé pour l'article (%d)\n",m.data1);

        
        // Attente du message de réponse (l'article suivant)
        // Cette partie dépend de l'implémentation de la gestion des signaux et des messages dans le client
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPrecedent_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
    // Vérifie si l'article actuel est défini
    if (articleEnCours.id > 1)  // On ne peut pas avoir d'ID inférieur à 1
    {
        // Id de l'article précédent
        int idPrecedent = articleEnCours.id - 1;

        MESSAGE m;
        m.type = 1;  // Type de message pour le serveur
        m.requete = CONSULT;  
        m.data1 = idPrecedent;  
        m.expediteur = getpid();  // Le PID du client

        
        if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
        {
            perror("(CLIENT) Erreur lors de l'envoi de la requête CONSULT");
            exit(1);
        }
        fprintf(stderr, "Requete consult envoyé pour l'article (%d)\n",m.data1);

        // Attente du message de réponse (l'article précédent)
        // Cette partie dépend de l'implémentation de la gestion des signaux et des messages dans le client
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonAcheter_clicked()
{
    // TO DO (étape 5)
    // Envoi d'une requete ACHAT au serveur

    int quantite = getQuantite();
    if(quantite <= 0)
    {
      dialogueErreur("Erreur","la quantite doit etre superieur a zero !!");
      return ; 
    }

    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  
    m.requete = ACHAT;  
    m.expediteur = getpid(); 
    m.data1 = articleEnCours.id; 
    sprintf(m.data2, "%d",quantite);

    fprintf(stderr, "(CLIENT %d) Envoi de la requête ACHAT pour l'article ID=%d, quantité=%d\n", getpid(),articleEnCours.id,getQuantite());

    // Envoi de la requête au serveur
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête ACHAT");
        exit(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSupprimer_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL au serveur
    int indice = getIndiceArticleSelectionne();
    if (indice == -1) 
    {
        dialogueErreur("Erreur", "Aucun article sélectionné pour la suppression.");
        return;
    }

    
    // Envoi de la requête CANCEL au serveur
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  
    m.requete = CANCEL;  
    m.data1 = indice;  
    m.expediteur=getpid();
    
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête CANCEL");
        return;
    }

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(0.0);

    
    // Envoi requete CADDIE au serveur
    MESSAGE caddieReq;
    memset(&caddieReq, 0, sizeof(MESSAGE));

    caddieReq.type = 1;  // Type de message pour le serveur
    caddieReq.requete = CADDIE;
    caddieReq.expediteur=getpid();

    if (msgsnd(idQ, &caddieReq, sizeof(MESSAGE) - sizeof(long), 0) == -1) {
        perror("(CLIENT) Erreur lors de l'envoi de la requête CADDIE");
        return;
    }

   

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonViderPanier_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL_ALL au serveur
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  
    m.requete = CANCEL_ALL;  
    m.expediteur = getpid();  

    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête CANCEL_ALL");
        return;
    }

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);



    // Envoi requete CADDIE au serveur
    MESSAGE caddieReq;
    memset(&caddieReq, 0, sizeof(MESSAGE));
    caddieReq.type = 1;
    caddieReq.requete = CADDIE;
    caddieReq.expediteur = getpid();

    if (msgsnd(idQ, &caddieReq, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête CADDIE");
        return;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPayer_clicked()
{
    // TO DO (étape 7)
    // Envoi d'une requete PAYER au serveur

    if(totalCaddie>0)
    {
      MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    m.type = 1;  
    m.requete = PAYER;  
    m.expediteur = getpid();  

    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
    {
        perror("(CLIENT) Erreur lors de l'envoi de la requête PAYER");
        return;
    }

    // Affichage du message de confirmation de paiement
    char tmp[100];
    sprintf(tmp, "Merci pour votre paiement de %.2f ! Votre commande sera livrée tout prochainement.", totalCaddie);
    dialogueMessage("Payer...", tmp);

    // Mise à jour du caddie (vider le panier)
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    }
    else
    {
      w->dialogueErreur("Erreur Paiement","Votre panier est vide veuillez selectionner des articles\n");
    }

    
    
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
  
    while(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),IPC_NOWAIT) != -1)  // !!! a modifier en temps voulu !!!
    {
      switch(m.requete)
      {
        case LOGIN :
                    {
                      fprintf(stderr, "Réception du signal SIGUSR1 (%d)\n",sig);
                      printf("Réponse du serveur : %s\n", m.data4);
                      if (m.data1 == 1) 
                      {
                        w->dialogueMessage("connexion Autorisée", m.data4);
                        w->loginOK();  // Si login réussi, activer les fonctionnalités
                        logged=true;

                        
                        MESSAGE consu;
                        consu.type = 1;  // Type de message pour le serveur
                        consu.requete = CONSULT;  // Requête de connexion
                        consu.expediteur = getpid();  // Utilise le PID du client comme expéditeur
                        consu.data1=1;

                        // Envoi du message de connexion au serveur
                        printf("Envois de la requete consult au serveur\n");
                        if (msgsnd(idQ, &consu, sizeof(MESSAGE) - sizeof(long), 0) == -1) 
                        {
                          perror("(CLIENT)  Erreur lors de l'envoi du message");
                          exit(1);
                        }
                      } 
                      else 
                      {
                        w->dialogueErreur("Erreur de connexion", m.data4);
                      }

                      break;
                    }
        case CONSULT : // TO DO (étape 3)
                    {
                      articleEnCours.id = m.data1;
                      strcpy(articleEnCours.intitule, m.data2);
                      articleEnCours.prix = m.data5;  
                      articleEnCours.stock = stoi(m.data3);
                      strcpy(articleEnCours.image, m.data4);

                      // Mise à jour de l'affichage avec les nouvelles données
                      w->setArticle(articleEnCours.intitule, articleEnCours.prix, articleEnCours.stock, articleEnCours.image);
                      break;
                    }
        case ACHAT : // TO DO (étape 5)
                  {
                    // Affichage du résultat de l'achat
                    if (atoi(m.data3)>0) 
                    {
                        char message[100];
                        sprintf(message, "%s unité(s) de %s achetées avec succès", m.data3, m.data2);
                        w->dialogueMessage("Achat réussi", message);

                       
                        MESSAGE caddie;
                        caddie.type = 1;
                        caddie.requete = CADDIE;
                        caddie.expediteur=getpid();
                        
                        if (msgsnd(idQ, &caddie, sizeof(MESSAGE) - sizeof(long), 0) == -1)
                        {
                            perror("Erreur lors de l'envoi de la requête CADDIE");
                            exit(1);
                        } 
                        w->videTablePanier();
                        totalCaddie=0.0; 
                    }
                    else
                    {
                      
                        w->dialogueErreur("Achat échoué", "Stock insuffisant !");
                    }
                    break;
                  }
                    

         case CADDIE : // TO DO (étape 5)
                    {
                      fprintf(stderr, "(CLIENT %d) Article reçu de CADDIE %d: ID=%d, %s, Quantité=%s\n", 
                                             getpid(),m.expediteur, m.data1,m.data2,m.data3);
                      // Ajout de l'article dans le panier et mise à jour du total
                      w->ajouteArticleTablePanier(m.data2, m.data5, atoi(m.data3));  // Ajout dans la table du panier
                      totalCaddie +=m.data5 * atoi(m.data3);  // Mise à jour du total
                      w->setTotal(totalCaddie);  // Mise à jour de l'affichage du total

                      break;
                    }
                     

         case TIME_OUT : // TO DO (étape 6)
                      {
                        fprintf(stderr, "TIME_OUT reçu !! \n");
    
                        w->logoutOK();

                        w->dialogueMessage("TIME_OUT", "vous avez été automatiquement déconnecté pour cause d’inactivité.");
                        
                      }
                    

         case BUSY : // TO DO (étape 7)
         {
           break;
         }
                    

         default :
                    break;
      }
    }
}
void handlerSIGUSR2(int sig) 
{  
  //fprintf(stderr, "SIGUSR2 reçu\n");

    if (pShm != NULL) 
    {
        w->setPublicite(pShm); 
    } 
    else 
    {
        fprintf(stderr, "(CLIENT %d) Erreur: pShm est NULL\n", getpid());
    }
      
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
