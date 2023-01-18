/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de
données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>
/* Pour utiliser inet_addr */
#include <arpa/inet.h>

#define PUITS     0
#define SOURCE    1
#define TCP       0
#define UDP       1

#define BAL       3
#define EMETTEUR  4
#define RECEPTEUR 5

#define TRUE      1
#define FALSE     0

#define COULEUR_ROUGE  "\033[0;31m"
#define COULEUR_VERTE  "\033[0;32m"
#define COULEUR_JAUNE  "\033[0;33m"
#define COULEUR_NORMAL "\033[0m"
#define COULEUR_ORANGE "\033[0;33m"
#define FORMAT_GRAS    "\033[1m"
#define COULEUR_BLEU   "\033[0;34m"

/* Variables UDP */

/* Variables TCP */
int sock_TCP;
int client_sock_TCP;
int c; 
struct sockaddr_in server_TCP;
struct sockaddr_in client_TCP;

typedef struct {
    int mode; //0=puits, 1=source 3=boite_aux_lettres 4=emmeteur 5=recepteur
    int protocol; //0=TCP par défaut, 1=UDP
    int nb_message; //Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception
    int lenght; //Taille des messages à envoyer ou à recevoir
    char dest[20]; //Nom de la machine de destination
    int port;
} config;

// Structure pour les boites aux lettres
typedef struct bal {
    char* user;
    struct lettre* lettres;
    struct bal* next;
    struct bal* prev;
} bal_t;

// Structure pour les lettres
typedef struct lettre {
    char* sender;
    char* message;
    struct lettre* next;
    struct lettre* prev;
} lettre_t;

// Pointeur vers le début de la liste des boites aux lettres
bal_t* balList = NULL;

// Ajoute une nouvelle boite aux lettres pour un user donné
void addbal(char* user) {
    bal_t* newbal = (bal_t*)malloc(sizeof(bal_t));
    newbal->user = (char*)malloc(strlen(user) + 1);
    strcpy(newbal->user, user);
    newbal->lettres = NULL;

    if (balList == NULL) {
        // C'est la première boite aux lettres de la liste
        newbal->prev = NULL;
        newbal->next = NULL;
        balList = newbal;
    }
    else {
        // Ajoute la boite aux lettres à la fin de la liste
        bal_t* lastbal = balList;
        while (lastbal->next != NULL) {
            lastbal = lastbal->next;
        }
        lastbal->next = newbal;
        newbal->prev = lastbal;
        newbal->next = NULL;
    }
}

// Ajoute une nouvelle lettre à la boite aux lettres d'un user donné
void addlettre(char* user, char* sender, char* message) {
    bal_t* bal = balList;
    while (bal != NULL) {
        if (strcmp(bal->user, user) == 0) {
            // Boite aux lettres trouvée, ajoute la lettre
            lettre_t* newlettre = (lettre_t*)malloc(sizeof(lettre_t));
            newlettre->sender = (char*)malloc(strlen(sender) + 1);
            strcpy(newlettre->sender, sender);
            newlettre->message = (char*)malloc(strlen(message) + 1);
            strcpy(newlettre->message, message);

            if (bal->lettres == NULL) {
                // C'est la première lettre de la boite aux lettres
                newlettre->prev = NULL;
                newlettre->next = NULL;
                bal->lettres = newlettre;
            }
            else {
                // Ajoute la lettre à la fin de la boite aux lettres
                lettre_t* lastlettre = bal->lettres;
                while (lastlettre->next != NULL) {
                    lastlettre = lastlettre->next;
                }
                lastlettre->next = newlettre;
                newlettre->prev = lastlettre;
                newlettre->next = NULL;
            }
            break;
        }
        bal = bal->next;
    }
}

// Affiche toutes les lettres d'une boite aux lettres d'un user donné
void displaylettres(char* user) {
    bal_t* bal = balList;
    while (bal != NULL) {
        if (strcmp(bal->user, user) == 0) {
            // Boite aux lettres trouvée, affiche les lettres
            lettre_t* lettre = bal->lettres;
            while (lettre != NULL) {
                printf("De : %s\n", lettre->sender);
                printf("Message : %s\n", lettre->message);
                lettre = lettre->next;
            }
            break;
        }
        bal = bal->next;
    }
}

void displaybal() {
    if (balList == NULL) {
        printf("Il n'y a pas de boite aux lettres à afficher.\n");
    }
    else {
        bal_t* bal = balList;
        while (bal != NULL) {
            printf("Boite aux lettres de l'utilisateur : %s\n", bal->user);
            lettre_t* lettre = bal->lettres;
            while (lettre != NULL) {
                printf("De : %s\n", lettre->sender);
                printf("Message : %s\n", lettre->message);
                lettre = lettre->next;
            }
            bal = bal->next;
        }
    }
}

// Libère la mémoire allouée pour toutes les boites aux lettres et les lettres
void freebal() {
    bal_t* bal = balList;
    while (bal != NULL) {
        lettre_t* lettre = bal->lettres;
        while (lettre != NULL) {
            lettre_t* nextlettre = lettre->next;
            free(lettre->sender);
            free(lettre->message);
            free(lettre);
            lettre = nextlettre;
        }
        bal_t* nextbal = bal->next;
        free(bal->user);
        free(bal);
        bal = nextbal;
    }
}

void decodeTrame(char* trame, char dest[], char length[], char nb_message[]) {
    char* token = strtok(trame, ";");
    strcpy(dest, token);

    token = strtok(NULL, ";");
    strcpy(length, token);

    token = strtok(NULL, ";");
    strcpy(nb_message, token);
}

char* codeTrame(char* dest, char* length, char* nb_message) {
    char* trame = malloc(strlen(dest) + strlen(length) + strlen(nb_message) + 3);
    sprintf(trame, "%s;%s;%s", dest, length, nb_message);
    return trame;
}

void afficher_config(config config) {
    printf(FORMAT_GRAS "========= CONFIGURATION =========" COULEUR_NORMAL "\n");

    printf(COULEUR_BLEU);

    if (config.mode == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

    switch (config.mode) {
        case SOURCE:
            printf("MODE: SOURCE\n");
            break;
        case PUITS:
            printf("MODE: PUITS\n");
            break;
        case BAL:
            printf("MODE: BAL\n");
            break;
        case EMETTEUR:
            printf("MODE: EMETTEUR\n");
            break;
        case RECEPTEUR:
            printf("MODE: RECEPTEUR\n");
            break;
        default:
            printf("Invalid mode\n");
    }

    if (config.protocol == TCP)
        printf("PROTOCOL: TCP\n");
    else
        printf("PROTOCOL: UDP\n");

    printf("NOMBRE DE MESSAGES: %d\n", config.nb_message);
    printf("LENGHT: %d\n", config.lenght);

    printf(COULEUR_NORMAL);
    printf(FORMAT_GRAS "=================================" COULEUR_NORMAL "\n");

}

void construire_message(char * message, char motif, config config) {
    int final_length = config.lenght + 5;
    
    for (int i = 0 ; i<config.lenght; i++) {
        message[i] = motif; 
    }

}

void afficher_message(char *message, config config) {
    printf("SOURCE : Envoi n°%d (%d)", 1, config.lenght);
    printf("[" "%s" "]\n", message);
}

char* itostr(int n) {
    char* str = malloc(sizeof(char) * 32);
    sprintf(str, "%d", n);
    return str;
}

int sendviaUDP(config config) {
    int sock;
    struct sockaddr_in server;
      
    //Création du socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // adresse IP du serveur puits
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
 
    char * message = malloc(config.lenght);

    // Envoi de données au serveur puits
    for (int i = 0; i < config.nb_message; i++) {
         
        
        construire_message(message, 'a'+i, config);

        printf("SOURCE : Envoi n°%d (%d)", i+1, config.lenght);
        printf("[" "%s" "]\n", message);

        // Utilisation de sendto pour envoyer les données
        if(sendto(sock, message, strlen(message), 0, (struct sockaddr *) &server, sizeof(server)) < 0) {
            printf(COULEUR_ROUGE "Erreur lors de l'envoi des données" COULEUR_NORMAL "\n");
        }

    }
    
    free(message);
    close(sock);

}

int recvfromUDP(config config) {
    int sock;
    int s_lenght = sizeof(struct sockaddr_in);
    struct sockaddr_in server;
    struct sockaddr_in client;
    char buffer[1000];
      
    //Création du socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    // Préparation de la struct sockaddr_in
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
      
    //Bind du socket
    if(bind(sock, (struct sockaddr*) &server, sizeof(server)) == -1) {
        printf(COULEUR_ROUGE "Erreur lors du bin du socket" COULEUR_NORMAL "\n");
    }
    printf("\n" COULEUR_VERTE "Socket bindé !" COULEUR_NORMAL "\n");

    int reception_count = 0;

    while(1) {
        // Réception des données
        if (recvfrom(sock, buffer, 1000, 0, (struct sockaddr *) &client, &s_lenght) == -1) {
            printf(COULEUR_ROUGE "Erreur lors de la réception des données" COULEUR_NORMAL "\n");
            break;
        }

        printf("PUITS : Reception n°%d (%d)", reception_count+1, config.lenght);
        printf("[" "%s" "]\n", buffer);

        reception_count++;
    }
    
    close(sock);

}

int sendviaTCP(config config) {
    int sock;
    struct sockaddr_in server;
    
    //Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // adresse IP du serveur puits
    server.sin_family = AF_INET;
    server.sin_port = htons(9000); // Port utilisé pour la communication
 
    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf(COULEUR_ROUGE "Erreur de connexion" COULEUR_NORMAL "\n");
    }
    else {
        printf("Connecté\n");
    }

    char * message = malloc(config.lenght);

    // Envoi de données au serveur puits
    for (int i = 0; i < config.nb_message; i++) {
        
        construire_message(message, 'a'+i, config);
        afficher_message(message, config);

        // Utilisation de sendto pour envoyer les données
        if( send(sock , message , strlen(message) , 0) < 0) {
            printf("Erreur lors de l'envoi des données");
        }
    }

    free(message);
    close(sock);
}

int initTCP(config config) {      
    //Création du socket
    sock_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_TCP == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
    // Préparation du struct sockaddr_in
    server_TCP.sin_family = AF_INET;
    server_TCP.sin_addr.s_addr = INADDR_ANY;
    server_TCP.sin_port = htons(9000);
      
    // Liason du socket
    if(bind(sock_TCP, (struct sockaddr *)&server_TCP , sizeof(server_TCP)) < 0) {
        printf(COULEUR_ROUGE "Erreur lors du bind du socket" COULEUR_NORMAL "\n");
    }
    else {
        printf("\n" COULEUR_VERTE "Socket bindé !" COULEUR_NORMAL "\n");
    }

    // Ecoute du socket
    listen(sock_TCP , 4);
}

int recvfromTCP(char *buffer, config config) {
    // Acceptation de la connexion
    printf(COULEUR_ORANGE "En attente de connexions..." COULEUR_NORMAL "\n");
    c = sizeof(struct sockaddr_in);

    // Acceptation de la connexion d'un client source
    client_sock_TCP = accept(sock_TCP, (struct sockaddr *)&client_TCP, (socklen_t*)&c);
    if (client_sock_TCP < 0) {
        printf(COULEUR_ROUGE "Erreur lors de l'acceptation de la connexion" COULEUR_NORMAL "\n");
    }
    else {
        printf(COULEUR_VERTE "Connexion acceptée" COULEUR_NORMAL "\n");
    }
    
    // Réception des données du client source
    int read_size;
    while((read_size = recv(client_sock_TCP , buffer, config.lenght , 0)) > 0 ) {
        afficher_message(buffer, config);
    }

    if(read_size == 0) {
        printf(COULEUR_JAUNE "Client déconnecté" COULEUR_NORMAL "\n");
        fflush(stdout);
    }
    else if(read_size == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la réception des données" COULEUR_NORMAL "\n");
    }

    close(client_sock_TCP);
}

int send_PID(config config) {
    // Trame PID : dest;lenght;nb_message
    
    //Création de la trame
    char * trame_PID = codeTrame(config.dest, itostr(config.lenght), itostr(config.nb_message));
    
    int sock;
    struct sockaddr_in server;
      
    //Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // adresse IP du serveur puits
    server.sin_family = AF_INET;
    server.sin_port = htons(9000); // Port utilisé uniquement pour l'envoi du PID
 
    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf(COULEUR_ROUGE "Erreur de connexion" COULEUR_NORMAL "\n");
    }
    else {
        printf("Connecté\n");
    }

    // Utilisation de sendto pour envoyer le PID
    if( send(sock , trame_PID , strlen(trame_PID) , 0) < 0) {
        printf("Erreur lors de l'envoi du PID");
    }

    close(sock);
}

int receive_PID(config config) {
    int s1;
    int client_sock;
    int c; 
    int read_size;
    struct sockaddr_in server;
    struct sockaddr_in client;
    
    char trame_PID[1000];
      
    //Création du socket
    s1 = socket(AF_INET, SOCK_STREAM, 0);
    if (s1 == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    // Préparation du struct sockaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9000); // Port utilisé uniquement pour l'envoi du PID
      
    // Liason du socket
    if(bind(s1, (struct sockaddr *)&server , sizeof(server)) < 0) {
        printf(COULEUR_ROUGE "Erreur lors du bind du socket" COULEUR_NORMAL "\n");
    }
    
    // Ecoute du socket
    listen(s1 , 4);

    // Acceptation de la connexion
    printf(COULEUR_ORANGE "En attente de connexions..." COULEUR_NORMAL "\n");
    c = sizeof(struct sockaddr_in);

    // Acceptation de la connexion d'un client source
    client_sock = accept(s1, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        printf(COULEUR_ROUGE "Erreur lors de l'acceptation de la connexion" COULEUR_NORMAL "\n");
    }

    recv(client_sock , trame_PID , 100 , 0);
    
    //Décodage de la trame
    printf("Trame reçue: %s\n", trame_PID);
    
    char dest[100], length[100], nb_message[100];
    decodeTrame(trame_PID, dest, length, nb_message);

    config.lenght = atoi(length); // Erreur : config.lenght n'est pas modifié
    config.nb_message = atoi(nb_message);
    printf("Message lenght updated: %d\n", config.lenght);
    printf("Number of messages updated: %d\n", config.nb_message);

    close(s1);
}

int emettre(config config) {
    //Envoyer PID de la taille du message
    printf("Envoi du PID\n");
    send_PID(config);

    sleep(1);

    //Envoyer message
    sendviaTCP(config);
}

int recevoir(config config)
{

}

int bal(config config) {
    // Recevoir le PID
    // Si PID = Emetteur alors on recoit les messages suivants
    // Si PID = Recepteur alors on envoie les messages adéquats


}

void main (int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
    char buffer[1000];

    config config = {-1, TCP, 10, 30, "dummy", 9999};

	while ((c = getopt(argc, argv, "pn:sberul:")) != -1) {
		switch (c) {
		case 'p':
			if (config.mode == SOURCE) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1);
			}
			config.mode = PUITS;
			break;

		case 's':
			if (config.mode == PUITS) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1) ;
			}
			config.mode = SOURCE;
			break;

        case 'b':
            config.mode = BAL;
            break;
        
        case 'e':
            config.mode = EMETTEUR;
            break;

        case 'r':
            config.mode = RECEPTEUR;
            break;

		case 'n':
			config.nb_message = atoi(optarg);
            printf("optarg: %s\n", optarg);
			break;

        case 'u':
            config.protocol = UDP;
            break;

        case 'l':
            config.lenght = atoi(optarg);
            break;

		default:
			printf("usage: cmd [-p|-s][-n ##]\n");
			break;
		}
	}

    afficher_config(config);

    if (config.mode == SOURCE) {
        if (config.protocol == UDP) {
            printf(COULEUR_VERTE "Envoi de données via UDP\n" COULEUR_NORMAL);
            sendviaUDP(config);
        }
        else {
            printf(COULEUR_VERTE "Envoi de données via TCP\n" COULEUR_NORMAL);
            sendviaTCP(config); 
        }
    }
    else if (config.mode == PUITS) {
        if (config.protocol == UDP) {
            printf("Receiving from UDP");
            recvfromUDP(config);
        }
        else {
            initTCP(config);
            while (1) {
                printf("Receiving from TCP");
                recvfromTCP(buffer, config); //Retourne le dernier message reçu dans le buffer
            }
        }
    }
    else if (config.mode == BAL) {
        bal(config);
    }
    else if (config.mode == EMETTEUR) {
        initTCP(config);
        emettre(config);
    }
    else if (config.mode == RECEPTEUR) {
        recevoir(config);
    }
}


