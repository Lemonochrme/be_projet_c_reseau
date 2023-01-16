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

#define PUITS 0
#define SOURCE 1
#define TCP 0
#define UDP 1

#define TRUE 1
#define FALSE 0

#define COULEUR_ROUGE "\033[0;31m"
#define COULEUR_VERTE "\033[0;32m"
#define COULEUR_JAUNE "\033[0;33m"
#define COULEUR_NORMAL "\033[0m"
#define COULEUR_ORANGE "\033[0;33m"

typedef struct {
    int mode; //0=puits, 1=source
    int protocol; //0=TCP, 1=UDP
    int nb_message; //Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception
    int lenght; //Taille des messages à envoyer ou à recevoir, par défaut : 1000 octets
} config;

void afficher_config(config config) {
	if (config.mode == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

    if (config.mode == SOURCE)
        printf(COULEUR_VERTE "MODE SOURCE\n" COULEUR_NORMAL);
    else
        printf(COULEUR_VERTE "MODE PUITS\n" COULEUR_NORMAL);

    if (config.protocol == TCP)
        printf(COULEUR_JAUNE "PROTOCOL TCP\n" COULEUR_NORMAL);
    else
        printf(COULEUR_JAUNE "PROTOCOL UDP\n" COULEUR_NORMAL);

    if (config.nb_message != -1) {
        if (config.mode == SOURCE)
            printf("Nombre de tampons à envoyer : %d\n", config.nb_message);
        else
            printf("Nombre de tampons à recevoir : %d\n", config.nb_message);
    } 
    else {
        if (config.mode == SOURCE) {
            config.nb_message = 10 ;
            printf("Nombre de tampons à envoyer = 10 par défaut\n");
        } 
        else
            printf("Nombre de tampons à envoyer = infini\n");

    }
}

void construire_message(char * message, char motif, int lg) {
    for (int i = 0 ; i<lg; i++) {
        message[i] = motif; 
    }
}

void afficher_message(char *message, int lg) {
	printf("message construit : ");

	for (int i = 0; i<lg; i++) {
		printf("%c", message[i]); 
        printf("\n");
	}
}

int sendviaUDP(char *message, config config) {
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
 
    // Envoi de données au serveur puits
    for (int i = 0; i < config.nb_message; i++) {
         
        message = malloc(config.lenght);
        construire_message(message, 'a'+i, 20);

        // Utilisation de sendto pour envoyer les données
        if(sendto(sock, message, strlen(message), 0, (struct sockaddr *) &server, sizeof(server)) < 0) {
            printf(COULEUR_ROUGE "Erreur lors de l'envoi des données" COULEUR_NORMAL "\n");
        }
    }
    
    free(message);
    close(sock);

}

char * recvfromUDP(config config) {
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
    printf(COULEUR_VERTE "Socket bindé !" COULEUR_NORMAL "\n");

    int packets_count = 0;

    while(1) {
        // Réception des données
        if (recvfrom(sock, buffer, 1000, 0, (struct sockaddr *) &client, &s_lenght) == -1) {
            printf(COULEUR_ROUGE "Erreur lors de la réception des données" COULEUR_NORMAL "\n");
            break;
        }
            packets_count++;
            printf("%d::", packets_count);
            printf("[" "%s" "]", buffer);
            printf("::Données reçues de %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    }
    
    close(sock);

}

int sendviaTCP(char *message, config config) {
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
        printf("Erreur de connexion\n");
    }
    printf("Connecté\n");

    // Envoi de données au serveur puits
    for (int i = 0; i < config.nb_message; i++) {
         
        message = malloc(config.lenght);
        construire_message(message, 'a'+i, 20);

        // Utilisation de sendto pour envoyer les données
        if( send(sock , message , strlen(message) , 0) < 0) {
            printf("Erreur lors de l'envoi des données");
        }
    }

    free(message);
    close(sock);
}

char * recvfromTCP(config config) {
    int sock;
    int client_sock;
    int c; 
    int read_size;
    struct sockaddr_in server;
    struct sockaddr_in client;
    char buffer[1000];
      
    //Création du socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la création du socket" COULEUR_NORMAL "\n");
    }
      
    // Préparation du struct sockaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9000);
      
    // Liason du socket
    if(bind(sock, (struct sockaddr *)&server , sizeof(server)) < 0) {
        printf(COULEUR_ROUGE "Erreur lors du bin du socket" COULEUR_NORMAL "\n");
    }
    printf(COULEUR_VERTE "Socket bindé !" COULEUR_NORMAL "\n");

    // Ecoute du socket
    listen(sock , 4);

    // Acceptation de la connexion
    printf(COULEUR_ORANGE "En attente de connexions..." COULEUR_NORMAL "\n");
    c = sizeof(struct sockaddr_in);

    // Acceptation de la connexion d'un client source
    client_sock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c);
    if (client_sock < 0) {
        printf(COULEUR_ROUGE "Erreur lors de l'acceptation de la connexion" COULEUR_NORMAL "\n");
    }
    printf(COULEUR_VERTE "Connexion acceptée" COULEUR_NORMAL "\n");

    // Réception des données du client source
    while((read_size = recv(client_sock , buffer , 2000 , 0)) > 0 ) {
        printf("Données reçues de %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        printf("Contenu: %s\n", buffer);
    }

    if(read_size == 0) {
        puts(COULEUR_JAUNE "Client déconnecté" COULEUR_NORMAL "\n");
        fflush(stdout);
    }
    else if(read_size == -1) {
        printf(COULEUR_ROUGE "Erreur lors de la réception des données" COULEUR_NORMAL "\n");
    }

    close(sock);
}


void main (int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;

    config config = {-1, -1, -1, 20};

	while ((c = getopt(argc, argv, "pn:su")) != -1) {
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

		case 'n':
			config.nb_message = atoi(optarg);
			break;

        case 'u':
            config.protocol = UDP;
            printf("azodihazoihd %d,", config.protocol);
            break;

        case 't':
            config.protocol = TCP;
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
            sendviaUDP("Hello from UDP !", config);
        }
        else {
            printf(COULEUR_VERTE "Envoi de données via TCP\n" COULEUR_NORMAL);
            sendviaTCP("Hello from TCP !", config); 
        }
    }
    else {
        if (config.protocol == UDP) {
            printf("Receiving from UDP");
            recvfromUDP(config);
        }
        else {
            printf("Receiving from TCP");
            recvfromTCP(config);
        }
    }
}


