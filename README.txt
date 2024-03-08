                                                    -------README-------
Voici notre projet final, concluant le second semestre en Système d'exploitation en troisième année de licence.
Ce projet a pour but de créer un serveur permettant à plusieurs clients de se connecter et de discuter ensemble.


                                                    ------Compilation------
Pour compiler notre programme voici les instructions :

1/ Ouvrir une fenêtre pour compiler le serveur : gcc -Wall serveur_projet.c -o serveur
Pour exécuter  lancer : ./serveur

2/ Ouvrir jusqu'à 10 autres fenêtres qui correspondent aux clients (tester avec 3 fenêtres ou plus ) : gcc -Wall client_projet.c -o client
Pour exécuter  chaque client lancer : ./client et entrer le nom d'utilisateur de chaque client

ATTENTION : TOUJOURS ENTRER LE NOM D'UTILISATEUR APRÈS AVOIR EXECUTER CHAQUE CLIENT



Pour afficher la matrice des messages dans le serveur : CTRL+C puis on vous demandera de choisir entre arrêter le programme et afficher la matrice.

Pour interrompre l'affichage des messages pour chaque client : CTRL+C. 

Si le client veut se déconnecter il suffit de fermer la fenêtre du client.
