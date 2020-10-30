/* Server Offline Messenger.c - Server TCP concurent care deserveste clientii
   printr-un mecanism de prethread-ing cu blocarea mutex de protectie a lui accept();
*/

  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <errno.h>
  #include <unistd.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdlib.h>
  #include <signal.h>
  #include <pthread.h>

    // Portul folosit
    #define PORT 2911

// Codul de eroare returnat de anumite apeluri
extern int errno;

static void * treat(void * ); // Functia executata de fiecare thread ce realizeaza comunicarea cu clientii

typedef struct {
    pthread_t idThread; // Id-ul thread-ului
    int thCount; // Numarul de conexiuni servite
}
Thread;

Thread * threadsPool; // Un vector de structuri Thread

int sd; // Descriptorul de socket de ascultare
int nthreads; // Numarul de fire de executie
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; // Variabila mutex ce va fi partajata de threaduri

void raspunde(int cl, int idThread);

// Verificarea dateleor de autentificare.
int autentificare(char username[], char password[]) {
    FILE * file = fopen("accounts.txt", "r");
    int exista = 0;
    if (file != NULL) {
        char linie[128];
        while (fgets(linie, sizeof linie, file) != NULL) {
            if (strlen(username) + strlen(password) == strlen(linie) - 2) {
                int i, egal = 1;
                for (i = 0; i < strlen(linie) - 1; i++) {
                    if ((i < strlen(username) && username[i] != linie[i]) || (i > strlen(username) && password[i - strlen(username) - 1] != linie[i])) {
                        egal = 0;
                        break;
                    }
                }
                if (egal) {
                    exista = 1;
                    break;
                }
            }
        }
        fclose(file);
    }
    return exista;
}

// Afisarea listei de prieteni.
void lista_prieteni(int cl, int idThread, char username[10]) {
    char nume_fisier[25];
    sprintf(nume_fisier, "%s/prieteni.txt", username);
    FILE * file = fopen(nume_fisier, "r");
    if (file != NULL) {
        char linie[128], newlinie[128];
        int lungime;
        bzero(linie, sizeof(linie));
        //for(int lungime=0;linie[lungime]!=' ';){lungime+=1;printf("%d\n",lungime);}
        while (fgets(linie, sizeof linie, file) != NULL) {
            int lungime = 0;
            while (linie[lungime] != ' ') {
                lungime += 1;
            }
            bzero(newlinie, sizeof(newlinie));
            strncpy(newlinie, linie, lungime);

            if (write(cl, & lungime, sizeof(int)) <= 0) {
                printf("[Thread %d] ", idThread);
                perror("[Thread]Eroare la write() catre client.\n");
            } else
                printf("[Thread %d] Lungimea \"%d\" a fost trasmisa cu succes.\n", idThread, lungime);

            if (write(cl, linie, lungime) <= 0) {
                printf("[Thread %d] ", idThread);
                perror("[Thread]Eroare la write() catre client.\n");
            } else
                printf("[Thread %d] Linia \"%s\" a fost trasmisa cu succes.\n", idThread, newlinie);
        }
        bzero(newlinie, sizeof(newlinie));
        bzero(linie, sizeof(linie));
        lungime = 0;
        if (write(cl, & lungime, sizeof(int)) <= 0) {
            printf("[Thread %d] ", idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        } else
            printf("[Thread %d] Lista de prieteni a utilizatorului \"%s\" a fost afisata cu succes.\n", idThread, username);
    }
    fclose(file);

}

// Verificarea daca un utilizator se afla in lista de prieteni a altui utilizator.
int este_prieten(char username[], char me[]) {
    char nume_fisier[25];
    sprintf(nume_fisier, "%s/prieteni.txt", username);
    FILE * file = fopen(nume_fisier, "r");
    if (file != NULL) {
        char linie[128];
        while (fgets(linie, 128, file) != NULL) {
            if (!strncmp(linie, me, strlen(me))) {
                return 1;
            }
        }
        fclose(file);
    }
    return 0;
}

// Trimitere mesaj.
void trimite_mesaj(char destinatar[10], char mesaj[100], char username[10]) {
    if (este_prieten(destinatar, username)) {
        char nume_fisier[25];
        sprintf(nume_fisier, "%s/mesaje_%s.txt", destinatar, username);
        FILE * file = fopen(nume_fisier, "a+");
        if (file != NULL) {
            fprintf(file, "%s: %s\n", username, mesaj);
        }
        fclose(file);

        sprintf(nume_fisier, "%s/mesaje_%s.txt", username, destinatar);
        file = fopen(nume_fisier, "a+");
        if (file != NULL) {
            fprintf(file, "(Eu)%s: %s\n", username, mesaj);
        }
        fclose(file);

        sprintf(nume_fisier, "%s/prieteni.txt", destinatar);
        file = fopen(nume_fisier, "r+");
        if (file != NULL) {
            char linie[128];
            while (fgets(linie, 128, file) != NULL) {
                if (!strncmp(linie, username, strlen(username))) {
                    fseek(file, -5, SEEK_CUR);
                    fprintf(file, "(!)");
                }
            }
        }
        fclose(file);
    }
}

// Afiseaza mesajele.
void citeste_mesaje(int cl, int idThread, char destinatar[10], char username[10]) {
    char nume_fisier[25];
    char linie[128];
    int lungime;
    sprintf(nume_fisier, "%s/mesaje_%s.txt", username, destinatar);
    FILE * file = fopen(nume_fisier, "r");
    if (file != NULL) {
        while (fgets(linie, sizeof linie, file) != NULL) {
            lungime = 1;
            if (write(cl, & lungime, sizeof(int)) <= 0) {
                printf("[Thread %d] ", idThread);
                perror("[Thread]Eroare la write() catre client.\n");
            }
            if (write(cl, linie, sizeof(linie)) <= 0) {
                printf("[Thread %d] ", idThread);
                perror("[Thread]Eroare la write() catre client.\n");
            } else
                printf("[Thread %d] Linia \"%s\" a fost trasmisa cu succes.\n", idThread, linie);

        }
        fclose(file);
    } else {
        lungime = 1;
        if (write(cl, & lungime, sizeof(int)) <= 0) {
            printf("[Thread %d] ", idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        }
        strcpy(linie, "Utilizator inexistent.\n\n");
        if (write(cl, linie, sizeof(linie)) <= 0) {
            printf("[Thread %d] ", idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        } else
            printf("[Thread %d] Linia \"%s\" a fost trasmisa cu succes.\n", idThread, linie);

    }
    lungime = 0;
    if (write(cl, & lungime, sizeof(int)) <= 0) {
        printf("[Thread %d] ", idThread);
        perror("[Thread]Eroare la write() catre client.\n");
    }

    sprintf(nume_fisier, "%s/prieteni.txt", username);
    file = fopen(nume_fisier, "r+");
    if (file != NULL) {
        char linie[128];
        while (fgets(linie, 128, file) != NULL) {
            if (!strncmp(linie, destinatar, strlen(destinatar))) {
                fseek(file, -5, SEEK_CUR);
                fprintf(file, "   ");
            }
        }
    }
    fclose(file);
}

int main(int argc, char * argv[]) {
    system("clear");
    struct sockaddr_in server; // Structura folosita de server
    void threadCreate(int);

    if (argc < 2) {
        fprintf(stderr, "Eroare: Primul argument este numarul de fire de executie.");
        exit(1);
    }
    nthreads = atoi(argv[1]);
    if (nthreads <= 0) {
        fprintf(stderr, "Eroare: Numar de fire invalid.");
        exit(1);
    }
    threadsPool = calloc(sizeof(Thread), nthreads);

    // Crearea unui socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server] Eroare la socket().\n");
        return errno;
    }
    // Utilizarea optiunii SO_REUSEADDR
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, & on, sizeof(on));

    // Pregatirea structurilor de date
    bzero( & server, sizeof(server));

    // Umplem structura folosita de server
    // Stabilirea familiei de socket-uri
    server.sin_family = AF_INET;
    // Acceptam orice adresa
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // Utilizam un port utilizator
    server.sin_port = htons(PORT);

    // Atasam socketul
    if (bind(sd, (struct sockaddr * ) & server, sizeof(struct sockaddr)) == -1) {
        perror("[Server] Eroare la bind().\n");
        return errno;
    }

    // Punem serverul sa asculte daca vin clienti sa se conecteze
    if (listen(sd, 2) == -1) {
        perror("[Server] Eroare la listen().\n");
        return errno;
    }

    printf(" Nr threaduri: %d.\n", nthreads);
    fflush(stdout);
    int i;
    for (i = 0; i < nthreads; i++) threadCreate(i);

    // Servim in mod concurent clientii...folosind thread-uri
    for (;;) {
        printf("[Server] Asteptam la portul %d.\n", PORT);
        pause();
    }
};

void threadCreate(int i) {
    void * treat(void * );

    pthread_create( & threadsPool[i].idThread, NULL, & treat, (void * ) i);
    return; // Threadul principal returneaza
}

void * treat(void * arg) {
    int client;

    struct sockaddr_in from;
    bzero( & from, sizeof(from));
    printf("[Thread %d] Pornit.\n", (int) arg);
    fflush(stdout);

    for (;;) {
        int length = sizeof(from);
        pthread_mutex_lock( & mlock);
        printf("[Thread %d] Trezit.\n", (int) arg);
        if ((client = accept(sd, (struct sockaddr * ) & from, & length)) < 0) {
            perror("[Thread] Eroare la accept().\n");
        }
        pthread_mutex_unlock( & mlock);
        threadsPool[(int) arg].thCount++;

        raspunde(client, (int) arg); // Procesarea cererii
        // Am terminat cu acest client, inchidem conexiunea

        close(client);
    }
}

void raspunde(int cl, int idThread) {

    char msg[100], username[20], password[20], destinatar[20], mesaj[500];
    int lungime = 1;
    // Citim confirmarea de conectare
    if (read(cl, msg, sizeof(msg)) <= 0) {
        printf("[Thread %d]\n", idThread);
        perror("Eroare la read() de la client.\n");
    } else {
        printf("[Thread %d] %s\n", idThread, msg);
        fflush(stdout);
    }
    // Citim username
    if (read(cl, username, sizeof(username)) <= 0) {
        printf("[Thread %d]\n", idThread);
        perror("Eroare la read() de la client.\n");
    } else {
        printf("[Thread %d] Username: \"%s\"\n", idThread, username);
        fflush(stdout);
    }
    // Citim parola
    if (read(cl, password, sizeof(password)) <= 0) {
        printf("[Thread %d]\n", idThread);
        perror("Eroare la read() de la client.\n");
    } else {
        printf("[Thread %d] Password: \"%s\"\n", idThread, password);
        fflush(stdout);
    }
    // Verificam datele
    if (autentificare(username, password)) {
        printf("[Thread %d] Utilizatorul \"%s\" s-a connectat.\n", idThread, username);
        fflush(stdout);
        strcpy(msg, "V-ati autentificat cu succes.");
        if (write(cl, msg, sizeof(msg)) <= 0) {
            printf("[Thread %d] ", idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        } else
            printf("[Thread %d] Raspunsul a fost trasmis cu succes.\n", idThread);

        // Autentificare efectuata
        while (strcmp(destinatar, "/q")) {

            // Afisam lista de prieteni
            lista_prieteni(cl, idThread, username);

            // Citim destinatar
            if (read(cl, & lungime, sizeof(int)) <= 0) {
                printf("[Thread %d]\n", idThread);
                perror("Eroare la read() de la client.\n");
            } else {
                printf("[Thread %d] Citim lungimea \"%d\"\n", idThread, lungime);
                fflush(stdout);
            }
            bzero(destinatar, sizeof(destinatar));
            if (read(cl, destinatar, lungime) <= 0) {
                printf("[Thread %d]\n", idThread);
                perror("Eroare la read() de la client.\n");
            } else if (!strcmp(destinatar, "/q")) {
                printf("[Thread %d] \"%s\" s-a deconectat.\n", idThread, username);
                fflush(stdout);
            } else {
                printf("[Thread %d] Destinatar: \"%s\"\n", idThread, destinatar);
                fflush(stdout);
            }

            if (strcmp(destinatar, "/q")) {

                while (!strstr(mesaj, "/q")) {

                    // Afisam mesajele
                    citeste_mesaje(cl, idThread, destinatar, username);
                    // Citim mesajul
                    bzero(mesaj, sizeof(mesaj));
                    if (read(cl, mesaj, sizeof(mesaj)) <= 0) {
                        printf("[Thread %d]\n", idThread);
                        perror("Eroare la read() de la client.\n");
                    } else if (strstr(mesaj, "/q")) {
                        printf("[Thread %d] \"%s\" a parasit conversatia cu %s\n", idThread, username, destinatar);
                        fflush(stdout);
                    } else {
                        if (strlen(mesaj) > 1) {
                            printf("[Thread %d] Mesajul \"%s\" catre %s\n", idThread, mesaj, destinatar);
                            fflush(stdout);
                            trimite_mesaj(destinatar, mesaj, username);
                        }
                    }

                }
                bzero(mesaj, sizeof(mesaj));
            }
        }
    } else {
        printf("[Thread %d] Clientul a gresit datele de autentificare.\n", idThread);
        fflush(stdout);
        strcpy(msg, "Ati gresit datele de autentificare.");
        if (write(cl, msg, sizeof(msg)) <= 0) {
            printf("[Thread %d] ", idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        } else
            printf("[Thread %d] Raspunsul a fost trasmis cu succes.\n", idThread);

    }
}
