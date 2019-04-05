#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <iostream>
#include <pthread.h>
#include <chrono>
#include "pugixml.cpp"

#define PORT 2024
#define NUM_CLIENTS 10000
extern int errno;

//clasa in care vom salva datele clientilor
class Client {
    int ok;
    long socket;
    char nume[25];
    bool logat;
    public:
        Client() {
            ok = 0;
            socket = 0;
            logat = false;
        }
    int getOk() {
        return ok;
    }
    long getSocket() {
        return socket;
    }
    void setSocket(int s) {
        socket = s;
    }
    void increaseOk() {
        ok++;
    }
    void setName(char name[]) {
        strcpy(nume, name);
    }
    char * getName() {
        return nume;
    }
    bool isLogat() {
        return logat;
    }
    void setLog(bool log) {
        logat = log;
    }
};
Client clienti[NUM_CLIENTS];

//primul si ultimul client
struct param
{
    long start,finish;
};

//datele unei intrebari
struct intrebare
{
    char detail[7][100];
};

intrebare question[10];

//salvam intrebarile
void getQuestions()
{
    pugi::xml_document doc;
    if (!doc.load_file("intrebari.xml")) {
        printf("Eroare la deschiderea fisierului XML\n");
        fflush(stdout);
    }
    int nr=0;
    int i=1;
    for(pugi::xml_node tool = doc.child("quiz").child("problem");tool;tool = tool.next_sibling("problem"))
    {
        i=1;
        strcat(question[nr].detail[i++],tool.child_value("question"));
        strcpy(question[nr].detail[i++],tool.child_value("answerA"));
        strcpy(question[nr].detail[i++],tool.child_value("answerB"));
        strcpy(question[nr].detail[i++],tool.child_value("answerC"));
        strcpy(question[nr].detail[i++],tool.child_value("answerD"));
        strcpy(question[nr].detail[i++],tool.child_value("correct"));
        nr++;
    }
    
}

//functie data thread-urilor atunci cand se conecteaza la server
void * clientLogin1(void * clientId) {
    
    long tid;
    tid = (long) clientId;
    char msgServer[100] = " ";
    bzero(msgServer, 100);
    //le spunem sa astepte pana se termina limita de timp
    strcat(msgServer, "Asteptam sa se conecteze concurentii...\n");
    fflush(stdout);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    return nullptr;
}


//functie ce va fi data thread-urilor pentru a-si introduce numele
void * clientLogin2(void * clientId) {
    
    long tid;
    tid = (long) clientId;
    char msgServer[100] = " ";
    //buffer pentru citirea numelui
    char nume[25];

    bzero(msgServer, 100);
    strcat(msgServer, "Introduceti numele dumneavoastra:");
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        close(clienti[tid].getSocket());
        return nullptr;
    }

    bzero(nume, 25);
    if (read(clienti[tid].getSocket(), nume, 25) <= 0) {
        printf("Eroare la citire in thread\n");
        fflush(stdout);
        close(clienti[tid].getSocket());
        return nullptr;
    }

    clienti[tid].setName(nume);

    bzero(msgServer, 100);
    strcat(msgServer, "Asteptam ca toti ceilalti concurenti sa isi introduca numele...\n");
    fflush(stdout);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    return nullptr;
}

//structura ce contine descriptorul clientului caruia trebuie sa ii trimitem intrebarea "intrebare"
struct cIntrebare
{
    long id;
    int intrebare;
};


//functia ce va fi data thread-urilor pentru a trimite intrebarile clientilor
void * clientPlay(void * arg) {
    
    struct cIntrebare *p=(struct cIntrebare*) arg;
    //id-ul clientului folosit pentru clasa
    long tid=p->id;
    int intrebareActuala=p->intrebare;
    

    struct timeval select_time;

    fd_set read_fds;
    FD_ZERO( & read_fds);
    FD_SET(clienti[tid].getSocket(), & read_fds);

    //buffer pentru intrebari/raspunsuri pentru client
    char msgServer[100] = " ";
    //raspunsul corect/raspunsul primit
    char corect[1], primit[5];

    //declarare pentru timer
    std::chrono::steady_clock::time_point tend;

    //trimitem clientuli:
    
    //intrebarea
    sleep(1);
    strcat(msgServer, question[intrebareActuala].detail[1]);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    //raspunsul A
    sleep(1);
    bzero(msgServer, 100);
    strcat(msgServer, question[intrebareActuala].detail[2]);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    //raspunsul B
    sleep(1);
    bzero(msgServer, 100);
    strcat(msgServer,question[intrebareActuala].detail[3]);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    //raspunsul C
    sleep(1);
    bzero(msgServer, 100);
    strcat(msgServer,question[intrebareActuala].detail[4]);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    //raspunsul D
    sleep(1);
    bzero(msgServer, 100);
    strcat(msgServer, question[intrebareActuala].detail[5]);
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }

    //cerem raspunsul
    sleep(1);
    bzero(msgServer, 100);
    strcat(msgServer, "Raspuns final:");
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }

    //salvam raspunsul corect
    strcpy(corect, question[intrebareActuala].detail[6]);

    bzero(primit, 5);

    //setam finalul timerului peste 30 secunde
    tend = std::chrono::steady_clock::now() + std::chrono::seconds(30);

    //citim raspunsul
    while (std::chrono::steady_clock::now() < tend) {
        select_time.tv_sec = 0;
        select_time.tv_usec = 100000;
        FD_ZERO( & read_fds);
        FD_SET(clienti[tid].getSocket(), & read_fds);
        if (select(clienti[tid].getSocket() + 1, & read_fds, NULL, NULL, & select_time) < 0) {
            printf("Eroare la select\n");
            fflush(stdout);
        }
        if (FD_ISSET(clienti[tid].getSocket(), & read_fds)) {
            if (read(clienti[tid].getSocket(), primit, 5) <= 0) {
                printf("Eroare la citire in thread\n");
                fflush(stdout);
                clienti[tid].setLog(false);
                close(clienti[tid].getSocket());
                return nullptr;
            } else {
                break;
            }
        }
    }

    bzero(msgServer, 100);
    //Verificam daca a raspuns peste timpul limita sau daca a raspuns corect
    if (std::chrono::steady_clock::now() < tend) {
        if (corect[0] == toupper(primit[0])) {
            strcat(msgServer, "Corect!");
            clienti[tid].increaseOk();
        } else strcat(msgServer, "Gresit!");
    } else {
        strcat(msgServer, "Ai trecut peste timpul limita!");
    }
    //Trimitem feedback
    if (send(clienti[tid].getSocket(), msgServer, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        clienti[tid].setLog(false);
        close(clienti[tid].getSocket());
        return nullptr;
    }

    return nullptr;
}

//cauta clientul cu cel mai mare punctaj
int find_Winner(int startClients,int numberClients) {
    
    int max = 0, winner = 0;
    for (int i = startClients; i < numberClients; i++) {
        if (clienti[i].getOk() > max && clienti[i].isLogat() == true) {
            max = clienti[i].getOk();
            winner = i;
        }
    }
    return winner;
}

//structura cu descriptorul clientului caruia trebuie sa ii trimitem mesajul cu jucatorul care a castigat
struct win
{
    long id;
    char mesaj[100];
};

//functie ce va fi data threadurilor pentru a afisa castigatorul
void * printWinner(void * arg) {
    struct win *p=(struct win*) arg;
    long tid=p->id;
    if (send(clienti[tid].getSocket(), p->mesaj, 100, MSG_NOSIGNAL) <= 0) {
        printf("Eroare la scriere in thread\n");
        fflush(stdout);
        close(clienti[tid].getSocket());
        return nullptr;
    }
    pthread_exit(NULL);
}

//functie ce va fi data thread-urilor pentru runda lor de joc
void * startGame(void * arg) {
    
    //primul si ultimul client din runda
    long start,finish;
    
    struct param *p=(struct param*) arg;
    start=p->start;
    finish=p->finish;
    //elibereaza resursele la finalul executiei
    pthread_detach(pthread_self());
    //numarul de threaduri necesare pentru actiuni
    pthread_t thread[finish - start];
    //index pentru thread
    int nr = 0;
    //mesaj de eroare pentru threaduri
    int threadErr;
    //pointer pentru a trimite parametrii la alte threaduri
    void * ptr;
    
    //facem rost de numele clientilor
    for(int i=start;i<finish;i++)
    {
        ptr = reinterpret_cast < void * > (i);
        threadErr = pthread_create( & thread[i], NULL, clientLogin2, ptr);
        if (threadErr) {
                printf("Unable to create thread\n");
                fflush(stdout);
            }
    }
    //asteptam ca toti sa introduca numele
    for(int i=start;i<finish;i++)
        pthread_join(thread[i],NULL);
    
    printf("Clientii s-au conectat\n");
    fflush(stdout);
    
    struct cIntrebare *argum=(struct cIntrebare*)malloc(sizeof(struct cIntrebare));
    //trimitem intrebarea actuala clientilor
    for (int intrebareActuala = 0; intrebareActuala < 10; intrebareActuala++) {
        nr = 0;
        for (int i = start; i < finish; i++) {
            (*argum).id=i;
            //intrebam numai clientii logati
            if (clienti[i].isLogat() == true)
            {
                (*argum).intrebare=intrebareActuala;
                threadErr = pthread_create( & thread[nr], NULL, clientPlay, argum);
                nr++;
                sleep(0.1);
                
            }
            if (threadErr) {
                printf("Unable to create thread\n");
                fflush(stdout);
            }
        }

        //asteptam ca toti clientii sa termine intrebarea
        for (int i = 0; i < finish - start; i++) {
            pthread_join(thread[i], NULL);
        }
    }

    printf("Aflam cine este castigatorul...\n");
    fflush(stdout);

    int winner = find_Winner(start,finish);
    char castigator[100] = " ";
    //mesajul cu castigatorul jocului
    bzero(castigator, 100);
    strcat(castigator, "Castigatorul este ");
    strcat(castigator, clienti[winner].getName());
    strcpy(castigator + strlen(castigator) - 1, " cu ");
    char append[2];
    sprintf(append, "%d", clienti[winner].getOk());
    strcat(castigator, append);
    strcat(castigator, " puncte!");

    sleep(5);//suspans

    printf("Comunicam clientilor cine a castigat runda\n");
    fflush(stdout);
    struct win * argument=(struct win*)malloc(sizeof(struct win));
    strcpy((argument->mesaj),castigator);
    
    //trimitem rezultatul tuturor clientilor
    for (int i = start; i < finish; i++) {
        if (clienti[i].isLogat() == true) {
            argument->id=i;
            pthread_create( & thread[i], NULL, printWinner, argument);
            pthread_join(thread[i], NULL);
        }
    }
    
    printf("Joc Terminat!\n");
    fflush(stdout);
}

int main() {
    pthread_t thread[NUM_CLIENTS]; //thread pentru fiecare client
    pthread_t gameThread[NUM_CLIENTS];//thread pentru fiecare runda de joc
    fd_set read_fds; //descriptor pentru select()
    
    // structura folosita de server
    struct sockaddr_in server; 
    struct sockaddr_in from;
    
    int sd; //descriptorul de socket
    //salvam intrebarile
    getQuestions();
    
    //pointer pentru parametrii pentru thread
    void * ptr;
    //primul si ultimul client din runda
    long start1, finish1;
    // crearea unui socket 
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    // pregatirea structurilor de date 
    bzero( & server, sizeof(server));
    bzero( & from, sizeof(from));

    // umplem structura folosita de server
    // stabilirea familiei de socket-uri 
    server.sin_family = AF_INET;
    // acceptam orice adresa 
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // utilizam un port utilizator 
    server.sin_port = htons(PORT);

    // atasam socketul 
    if (bind(sd, (struct sockaddr * ) & server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    // punem serverul sa asculte daca vin clienti sa se conecteze 
    if (listen(sd, 3) < 0) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }
    //timerul pentru limita de timp in care se pot loga clientii la o runda de joc
    std::chrono::steady_clock::time_point tend;

    //numarul de clienti care vor fi conectati
    long clientsConnected = 0;

    //structura pentru timeout pentru select()
    struct timeval select_time;

    printf("[server]Asteptam la portul %d...\n", PORT);
    int length = sizeof(from);
    int threadErr;
    //descriptorul clientului
    long client;
    //index pentru gameThread
    long game=0;
    // servim in mod concurent clientii... 
    while (true) {
        //primul client din runda 
        start1 = clientsConnected;
        //timerul de conectare va fi de 1 minute
        tend = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        while (std::chrono::steady_clock::now() < tend) {
            //resetam timerul pana cand avem macar 2 clienti ca sa nu joace nimeni singur
            if(clientsConnected - start1 <2)
                tend = std::chrono::steady_clock::now() + std::chrono::seconds(60);
            //timeout pentru select
            select_time.tv_sec = 0;
            select_time.tv_usec = 100000;
            
            FD_ZERO( & read_fds);
            FD_SET(sd, & read_fds);
            
            if (select(sd + 1, & read_fds, nullptr, nullptr, & select_time) < 0) {
                printf("[server]Eroare la select\n");
                fflush(stdout);
            }

            //daca avem un client care vrea sa se conecteze il acceptam
            if (FD_ISSET(sd, & read_fds)) {
                if ((client = accept(sd, (struct sockaddr * ) & from, (socklen_t * ) & length)) == -1) {
                    printf("[server]Eroare la accept\n");
                    fflush(stdout);
                }
                //pornim thread-ul cu mesajul de asteptare
                ptr = reinterpret_cast < void * > (clientsConnected);
                threadErr = pthread_create( & thread[clientsConnected], NULL, clientLogin1, ptr);
                if (threadErr) {
                    printf("Unable to create thread\n");
                    fflush(stdout);
                }
                //salvam datele clientului si il setam ca logat
                clienti[clientsConnected].setSocket(client);
                clienti[clientsConnected].setLog(true);
                
                clientsConnected++;
            }
        }
        //ultimul client din runda
        finish1 = clientsConnected;
        //ne asiguram ca s-au terminat threadurile cu mesajul de asteptare
        for(int i=start1;i<finish1;i++)
            pthread_join(thread[i],NULL);
        struct param *arg=(struct param*)malloc(sizeof(struct param));
        (*arg).start=start1;
        (*arg).finish=finish1;
        //pornim runda de joc
        threadErr = pthread_create( &gameThread[game], NULL, startGame, arg);
        game++;
        if (threadErr) {
                printf("Unable to create thread mai\n");
                fflush(stdout);
            }
        sleep(5);
    }
}