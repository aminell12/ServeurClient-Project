#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

                /*---INITIALISATION DES VARIABLES ET STRUCTURE---*/
#define UNIX_PATH_MAX 108
#define BUFFER_SIZE 1024
//Structure qui définie un client
typedef struct {
    int sclient; //Descritpeur du fichier
    char username[BUFFER_SIZE]; //Nomb d'utilisateur
    int color;; //Code couleur propre à chaque client
} ClientInfo;

volatile sig_atomic_t interruption = 0; //Variable qui gère l'interruption de l'affichage
char messages[BUFFER_SIZE][BUFFER_SIZE]; // Variable qui va stocker les messages envoyés par les autres utilisateurs lors de l'interruption
int messageCount = 0; //Nombre de messages

//Liste de couleur pour chaque utilisateur
const char* colors[] = {
    "\033[33m",  // Jaune
    "\033[34m",  // Bleu
    "\033[35m",  // Magenta
    "\033[36m",  // Cyan
    "\033[1;31m",  // Rouge vif
    "\033[1;33m",  // Jaune vif
    "\033[1;34m",  // Bleu vif
    "\033[1;35m",  // Magenta vif
    "\033[1;36m",  // Cyan vif
    "\033[1;37m",  // Blanc
    "\033[90m",  // Gris foncé
    "\033[91m",  // Rouge foncé
    "\033[93m",   // Jaune foncé
};

                /*---INITIALISATION DES FONCTIONS ET THREADS---*/
//Fonction permettant d'interrompre l'affichage des messages en annonçant au client qu'il peut écrire
void gestionnaireInterruption(int signal) {
    if (signal == SIGINT) {
        interruption = 1;
        printf("\n\033[31mInterruption de l'affichage. Rédigez votre message.\033[0m\n");
    }
}
//Fonction permettant d'afficher les messages envoyés par les autres utilisateurs durant une interruption
void afficherMessages() {
    printf("\033[31mMessages reçus pendant l'interruption :\033[0m\n");
    char envoyeur[BUFFER_SIZE]; // Nom d'utilisateur de l'envoyeur
    char message_def[BUFFER_SIZE];
    //Bloc qui permet de recupérer le nom d'utilisateur et le messsage.
    for (int i = 0; i < messageCount; i++) {
        for (int j=0;j < strlen(messages[i]);j++){
            if (messages[i][j] != '\n'){
                message_def[j]=messages[i][j];
                }
            else{
                strcpy(envoyeur,&messages[i][j+1]);
            }
        }
        //Affiche le message
        printf("%s : %s\n",envoyeur, message_def);
        memset(message_def, 0, BUFFER_SIZE);
        memset(envoyeur, 0, BUFFER_SIZE);
    }
    memset(messages,0,BUFFER_SIZE);
    messageCount = 0;
}
//Thread qui va recevoir le message écrit par les autres clients, prend en argument la structure du client courant
void *receiveMessage(void *arg) {
    ClientInfo *clientInfo = (ClientInfo *)arg; // Structure du client
    int sclient = clientInfo->sclient; //récupère le descripteur de fichier
    char message[BUFFER_SIZE] = {0}; // Message brut (contenant le nom d'utilisateur du client)
    char message_def[BUFFER_SIZE] = {0}; // Message écrit par les clients
    char envoyeur[BUFFER_SIZE] = {0}; // Nom d'utilisateur

    while (1) {
        ssize_t bytes_received = read(sclient, message, BUFFER_SIZE);
        if (bytes_received == -1) {
            perror("Erreur lors de la lecture du message.\n");
            break;
        } else if (bytes_received == 0) {
            printf("\033[31mDéconnexion du serveur.\033[0m\n");
            exit(1);
        }
        //Bloc permettant de récupérer le nom d'utilisateur ainsi que le message reçus
        for (int i=0;i < strlen(message);i++){
          if (message[i] != '\n'){
            message_def[i]=message[i];
          }
          else{
            strcpy(envoyeur,&message[i+1]);
          }
        }
        //Ici on veut recevoir uniquement les messages des autres clients et non les notres(client courant), donc on verifie de quel client il s'agit, et si le message est non vide.
        if ((strcmp(clientInfo->username,envoyeur)!=0) && (strcmp(message_def,"")!=0)){
          if (interruption) { // On vérifie si l'utilisateur veut interrompre l'affichage des messages
                strncpy(messages[messageCount], message, BUFFER_SIZE);
                messageCount++;
            } else {
                printf("%s : %s\n" ,envoyeur,message_def);
            }
        }
        //Reinitialisation des buffers à 0 caractères
        memset(message, 0, BUFFER_SIZE);
        memset(message_def, 0, BUFFER_SIZE);
        memset(envoyeur, 0, BUFFER_SIZE);
    }
    pthread_exit(NULL);
}
//Thread prenant en argument la structure du client courant et qui envoie le message écrit par ce dernier aux autres clients
void *sendMessage(void *arg) {
    ClientInfo *clientInfo = (ClientInfo *)arg; // Structure du client
    int sclient = clientInfo->sclient; //récupère le descripteur de fichier dans lequel on écrit
    char message[BUFFER_SIZE] = {0}; // Message écrit par le client

    while (1) {
        fgets(message, BUFFER_SIZE, stdin); //Récupère le message écrit par le client
        strcat(message, clientInfo->username); //On concatène le nom d'utilisateur avec le message
        ssize_t bytes_sent = write(sclient, message, strlen(message) + 1); //écrit dans le descripteur de fichier
        if (bytes_sent == -1) {
            perror("Erreur lors de l'envoi du message.\n");
            break;
        }
        memset(message, 0, BUFFER_SIZE); //Réinitialise le buffer à 0.
        //Si nous nous retrouvons pendant l'interruption, le client peut écrire son message, on remet à 0 interruption pour en sortir plus tard.
        //Puis on affiche les messages envoyés durant l'interruption.
        if (interruption){
            interruption = 0;
            afficherMessages();
        }
    }
    // Fermeture de la socket client
    close(sclient);
    pthread_exit(NULL);
}
            /*---MAIN---*/
int main(int argc, char *argv[]) {
    int sclient; // fichier descripteur pour socket client

    // Initialisation struct adresse du serveur
    struct sockaddr_un saddr = {0}; // Struct adresse du serveur
    saddr.sun_family = AF_UNIX;    // Address Family UNIX
    strcpy(saddr.sun_path, "./MySock"); // Chemin vers fichier socket

    // Création socket client
    sclient = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sclient == -1) {
        perror("Erreur lors de la création de la socket.\n");
        exit(1);
    }
    // Tentative de connexion au serveur
    if (connect(sclient, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
        perror("Erreur lors de la connexion au serveur.\n");
        exit(1);
    }

    // Création des threads pour recevoir et envoyer les messages
    pthread_t receiveThread, sendThread;

    ClientInfo *clientInfo = malloc(sizeof(ClientInfo));
    clientInfo->sclient = sclient;
    // Demander le nom d'utilisateur au client
    char user[BUFFER_SIZE];
    printf("\033[31mEntrez votre nom d'utilisateur : \033[0m");
    scanf("%s", user);

    //Donne une couleur pour chaque client
    srand(time(NULL));
    clientInfo->color = rand() % (sizeof(colors) / sizeof(colors[0]));

    strcpy(clientInfo->username, colors[clientInfo->color]); //Pour donner une couleur propre à chaque client

    strcat(clientInfo->username,user); //copie le nom d'utilisateur
    strcat(clientInfo->username,"\033[0m");


    //Permet d'interrompre l'affichage des messages des autres utilisateurs
    signal(SIGINT, gestionnaireInterruption);

    if (pthread_create(&receiveThread, NULL, receiveMessage, clientInfo) != 0) {
        perror("Erreur lors de la création du thread de réception.\n");
        free(clientInfo);
        close(sclient);
        exit(1);
    }

    if (pthread_create(&sendThread, NULL, sendMessage, clientInfo) != 0) {
        perror("Erreur lors de la création du thread d'envoi.\n");
        free(clientInfo);
        close(sclient);
        exit(1);
    }

    // Attente de la fin des threads
    pthread_join(receiveThread, NULL);
    pthread_join(sendThread, NULL);

    // Fermeture de la socket client
    close(sclient);
    return 0;
}