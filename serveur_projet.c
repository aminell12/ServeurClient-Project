#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

                /*---INITIALISATION DES VARIABLES---*/
#define BUFFER_SIZE 1024 //Taille d'un buffer de texte
#define MAX_CLIENTS 10 //Nombre maximum de clients pouvant communiquer (peut être modifié)
#define MAX_MESSAGES 1000 //Nombre maximum de messages pouvant être envoyés

// Matrice contenant tous les messages envoyés par les utilisateurs
char messages[MAX_CLIENTS][MAX_MESSAGES][BUFFER_SIZE];
int nb_message[MAX_CLIENTS] = {0};
int terminate = 0;

int clientSockets[MAX_CLIENTS]; //Tableau de clients, afin de reconnaitre chaque client
int numClients = 0; //Nombre de clients connectés
pthread_mutex_t mutex; //Mutex pour bloquer aux ressources partagés entre les threads

                /*---INITIALISATION DES FONCTIONS ET THREADS---*/
//Fonction permettant d'afficher toute la matrice des messages envoyés depuis le début de la conversation
void gestionnaireInterruption(int signal) {
  printf("\nEntrer 1 si vous souhaitez afficher la matrice des messages.\n");
  printf("Entrer 2 si vous souhaitez arrêter le programme.\n");
  char m[BUFFER_SIZE];
  memset(m, '\0', BUFFER_SIZE);
  scanf("%s",m);
    if ((signal == SIGINT) && (m[0] == '1')) {
        printf("\n\033[32mVoici tous les messages envoyés : \033[0m\n");
        //bloc permettant d'afficher tous les messages de chaque utilisateur
        for (int i =0; i<numClients;i++){
            //affiche les messages de l'utilisateur i
          for (int j = 0; j<nb_message[i] ; j++){
            //S'il n'a rien envoyé on met 0
            printf("mess : %s \t",messages[i][j]);
          }
          printf("\n");
        }
    }
    //Cas ou l'utilisateur veut arrêter le programme
    else {
      terminate=1;
      unlink("MySock");
      exit(1);
    }
}

//Fonction qui permet d'envoyer le message écrit par un client à tous les autres clients, prend en argument le message en question
void sendMessageToAllClients(const char *message) {
    // Verrouiller l'accès aux ressources partagées
    pthread_mutex_lock(&mutex);

    // Parcourir tous les clients et envoie le message à tout le monde
    for (int i = 0; i < numClients; i++) {
        int clientSocket = clientSockets[i];
        if (write(clientSocket, message, strlen(message) + 1) <= 0) {
            perror("Erreur lors de l'envoi du message au client.\n");
        }
    }
    // Déverrouiller l'accès aux ressources partagées
    pthread_mutex_unlock(&mutex);
}
//Thread qui permet de gérer tous les clients, il prend en argument le descripteur de fichier sclient
void *handleClient(void *arg) {
    int sclient = *(int *)arg;
    char message[BUFFER_SIZE] = {0}; //Buffer qui contient le message ainsi que le nom d'utilisateur du client qui a envoyé le message
    char message_def[BUFFER_SIZE] = {0}; //Buffer qui contient uniquement le message envoyé
    while (1) {
        if (read(sclient, message, BUFFER_SIZE) <= 0) {
            // Erreur de lecture ou le client s'est déconnecté
            break;
        }
        //Bloc permettant de récuperer le message sans le nom d'utilisateur et le met dans message_def
        for (int i=0;i < strlen(message);i++){
          if (message[i] != '\n'){
            message_def[i]=message[i];
          }
        }
        //Bloc permettant de copier les messages envoyés dans la matrice de gestion de message
        for (int i=0;i<MAX_CLIENTS;i++){
          if (sclient==clientSockets[i]){
            strcpy(messages[i][nb_message[i]], message_def);
            nb_message[i]++;
          }
        }
        // Envoyer le message à tous les autres clients
        sendMessageToAllClients(message);

        //Reinitialisation des buffers à 0 caractères
        memset(message, '\0', BUFFER_SIZE);
        memset(message_def, '\0', BUFFER_SIZE);
    }
    // Fermer la connexion avec le client
    close(sclient);

    // Verrouiller l'accès aux ressources partagées
    pthread_mutex_lock(&mutex);
    // Bloc permettant de supprimer le socket client de la liste
    for (int i = 0; i < numClients; i++) {
        if (clientSockets[i] == sclient) {
            for (int j = i; j < numClients - 1; j++) {
                clientSockets[j] = clientSockets[j + 1];
            }
            numClients--;
            break;
        }
    }
    // Déverrouiller l'accès aux ressources partagées
    pthread_mutex_unlock(&mutex);

    printf("\033[32mClient déconnecté.\033[0m\n");

    pthread_exit(NULL);
}
            /*---MAIN---*/
int main(int argc, char *argv[]) {
    //Permet d'interrompre le programme et d'afficher les messages avec CTRL+C
    signal(SIGINT, gestionnaireInterruption);
    socklen_t caddrlen; // Taille adresse client

    // Initialisation struct addr serveur
    struct sockaddr_un saddr = {0};
    saddr.sun_family = AF_UNIX; // Address Family UNIX
    strcpy(saddr.sun_path, "./MySock"); // Chemin vers fichier socket local

    // Creation socket serveur
    int secoute = socket(AF_UNIX, SOCK_STREAM, 0); // File Descriptor pour socket serveur
    if (secoute == -1) {
        perror("Erreur lors de la création de la socket.\n");
        exit(1);
    }
    // Connection de la socket à l'adresse serveur
    if (bind(secoute, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
        perror("Erreur lors de la liaison de la socket à l'adresse serveur.\n");
        exit(1);
    }
    // Socket en attente de connexion (à l'écoute)
    if (listen(secoute, 5) == -1) {
        perror("Erreur lors de la mise en écoute de la socket.\n");
        exit(1);
    }
    printf("\033[32mServeur en attente de connexions...\033[0m\n");
    // Initialisation du mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Erreur lors de l'initialisation du mutex.\n");
        exit(1);
    }
    //Boucle infini qui permet les connexions au serveur/client
    while (terminate != 1) {
                    /*---BONUS pour afficher l'heure des connexions---*/
        time_t heureActuelle;
        struct tm *tempsLocal;
        char heure[9];
        // Obtenir l'heure actuelle
        heureActuelle = time(NULL);
        // Convertir l'heure en temps local
        tempsLocal = localtime(&heureActuelle);
        // Formater l'heure dans une chaîne
        strftime(heure, sizeof(heure), "%H:%M:%S", tempsLocal);
                    /*--- FIN BONUS---*/

        // Accepter les connexions
        struct sockaddr_un caddr;

        caddrlen = sizeof(caddr);

        // Accepter une connexion parmi les connexions en attente
        int sclient = accept(secoute, (struct sockaddr *)&caddr, &caddrlen);
        if (sclient == -1) {
            perror("Erreur lors de l'acceptation de la connexion.\n");
            continue;
        }
        //Bloc recupérant le nom d'utilisateur brut, puis le modifie pour enlever les caractères genants (ici ce sont les retours à la ligne)
        char username[BUFFER_SIZE];
        memset(username, '\0', BUFFER_SIZE);
        read(sclient, username, BUFFER_SIZE);
        char username_def[strlen(username)];
        for (int i=1; i< strlen(username)+1;i++){
            username_def[i-1]=username[i];
        }

        printf("\033[32mConnexion établie avec l'utilisateur : \033[0m%s \033[32mà\033[0m %s\n", username_def, heure);

        //Thread pour chaque client
        pthread_t clientThread;

        // Verrouiller l'accès aux ressources partagées
        pthread_mutex_lock(&mutex);

        // Ajouter le socket client à la liste
        clientSockets[numClients++] = sclient;

        // Démarrer un thread pour gérer le client
        if (pthread_create(&clientThread, NULL, handleClient, &sclient) != 0) {
            perror("Erreur lors de la création du thread client.\n");
            continue;
        }
        // Déverrouiller l'accès aux ressources partagées
        pthread_mutex_unlock(&mutex);
    }

    // Fermeture de la socket d'écoute
    close(secoute);
    unlink("MySock");

    // Destruction du mutex
    pthread_mutex_destroy(&mutex);

    return 0;
}
