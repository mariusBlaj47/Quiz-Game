#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <chrono>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

void error(const char * msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char * argv[]) {

    char buffer[256];
    struct timeval select_time;
    int sockfd; // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare 

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3) {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[client]Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sockfd, (struct sockaddr * ) & server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    //"Asteptam sa se conecteze concurentii..."
    bzero(buffer, 256);
    if (read(sockfd, buffer, 255) <= 0)
        error("[client]Eroare la citire\n");
    printf("%s\n", buffer);

    //"Introduceti numele:"
    bzero(buffer, 256);
    if (read(sockfd, buffer, 255) <= 0)
        error("[client]Eroare la citire\n");
    printf("%s\n", buffer);

    //Trimitem numele 
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    if (send(sockfd, buffer, strlen(buffer), 0) <= 0)
        error("[client]Eroare la trimitere\n");

    //"Asteptam sa se conecteze concurentii..."
    bzero(buffer, 256);
    if (read(sockfd, buffer, 255) <= 0)
        error("[client]Eroare la citire\n");
    printf("%s\n", buffer);

    char raspuns[100];
    fd_set read_fds;
    //timer pentru timpul limita de raspuns
    std::chrono::steady_clock::time_point tend;
    bool citit;
    
    //vom citi 10 intrebari
    for (int i = 1; i <= 10; i++) {
        
        FD_ZERO( & read_fds);
        FD_SET(sockfd, & read_fds);
        if (select(sockfd + 1, & read_fds, NULL, NULL, NULL) < 0)
            printf("[client]Eroare la select\n");
        
        //citeste intrebarea,cele 4 raspunsuri si "Raspuns final:"
        if (FD_ISSET(sockfd, & read_fds)) {
            for (int j = 1; j <= 6; j++) {
                bzero(buffer, 256);
                if (read(sockfd, buffer, 255) <= 0)
                    error("[client]Eroare la citire\n");
                printf("%s\n", buffer);
            }
            //timpul limita va fi 30
            tend = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            citit = false;
            //trimitem raspunsul
            do {
                select_time.tv_sec = 0;
                select_time.tv_usec = 100000;
                FD_ZERO( & read_fds);
                FD_SET(STDIN_FILENO, & read_fds);
                bzero(raspuns, 100);
                if (select(STDIN_FILENO + 1, & read_fds, NULL, NULL, & select_time) < 0)
                    printf("[client]Eroare la select\n");
                if (FD_ISSET(STDIN_FILENO, & read_fds)) {
                    fgets(raspuns, 100, stdin);
                    if (strlen(raspuns) > 2) {
                        printf("Introduceti o singura litera! \n");
                    } else {
                        if (send(sockfd, raspuns, strlen(raspuns), 0) <= 0)
                            error("[client]Eroare la trimitere\n");
                        citit = true;
                    }
                }
            } while (std::chrono::steady_clock::now() < tend && !citit);

            //citim feedback-ul (daca raspunsul este corect/gresit/nu am raspuns la timp
            bzero(buffer, 256);
            if (read(sockfd, buffer, 255) <= 0)
                error("[client]Eroare la citire\n");
            printf("%s\n", buffer);
        }
    }

    //Citim castigatorul
    bzero(buffer, 256);
    if (read(sockfd, buffer, 255) <= 0)
        error("[client]Eroare la citire\n");
    printf("%s\n", buffer);

    return 0;
}