/* Client Offline Messenger.c - Client TCP
   Trimite date serverului iar serverul trimite datele catre alti utilizatori.
*/

  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <errno.h>
  #include <unistd.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <netdb.h>
  #include <string.h>

    // Codul de eroare returnat de anumite apeluri
    extern int errno;

// Portul de conectare la server
int port;

void autentificare(int sd) {

    char username[20], password[20];

    // Citim username de la tastatura
    printf("Username: ");
    fflush(stdout);
    scanf("%s", username);

    // Trimitem username
    if (write(sd, username, sizeof(username)) <= 0) {
        perror("[client]Eroare la write() spre server.\n");
    }

    // Citim parola de la tastatura
    printf("Password: ");
    fflush(stdout);
    scanf("%s", password);

    // Trimitem parola
    if (write(sd, password, sizeof(password)) <= 0) {
        perror("[client]Eroare la write() spre server.\n");
    }
}
// Mesaje de primire
void watermark() {
    printf("\t\t\t\t\tOFFLINE MESSENGER\n\nAplicatie realizata de Corban Cristian\n\n");
    fflush(stdout);
}
// Interfata
void help() {
    printf("\t\t\t\t\tOFFLINE MESSENGER\n\nPentru a iesi tastati /q apoi apasati enter.\n\n");
    fflush(stdout);
}
// Functia care primeste lista de prieteni si o afiseaza.
void lista_prieteni(int sd) {
    int lungime = 1;
    char msg[128];

    while (lungime) {
        lungime = 0;
        if (read(sd, & lungime, sizeof(int)) < 0) {
            perror("[client]Eroare la read() de la server.\n");
        }
        if (lungime) {
            bzero(msg, sizeof(msg));
            if (read(sd, msg, lungime) < 0) {
                perror("[client]Eroare la read() de la server.\n");
            }
            printf("%s\n\n", msg);
            fflush(stdout);
        }
    }
}
// Functia care primeste mesajele si le afiseaza.
void mesaje(int sd) {
    int lungime = 1;
    char msg[128];

    while (lungime) {
        lungime = 0;
        if (read(sd, & lungime, sizeof(int)) < 0) {
            perror("[client]Eroare la read() de la server.\n");
        }
        if (lungime) {
            bzero(msg, sizeof(msg));
            if (read(sd, msg, sizeof(msg)) < 0) {
                perror("[client]Eroare la read() de la server.\n");
            }
            printf("%s", msg);
            fflush(stdout);
        }
    }

}

int main(int argc, char * argv[]) {
    int sd; // Descriptorul de socket
    struct sockaddr_in server; // Structura folosita pentru conectare
    // Mesajul trimis

    // Verificam argumentele
    if (argc != 3) {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        fflush(stdout);
        return -1;
    }

    // Stabilim portul
    port = atoi(argv[2]);

    // Cream socketul
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Eroare la socket().\n");
        return errno;
    }

    // Umplem structura folosita pentru realizarea conexiunii cu serverul
    // familia socket-ului
    server.sin_family = AF_INET;
    // Adresa IP a serverului
    server.sin_addr.s_addr = inet_addr(argv[1]);
    // Portul de conectare
    server.sin_port = htons(port);

    // Ne conectam la server
    if (connect(sd, (struct sockaddr * ) & server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    char destinatar[20], mesaj[500], msg[100];
    int lungime = 1;
    int test = 1;

    strcpy(msg, "Client connectat.");
    if (write(sd, msg, sizeof(msg)) <= 0) {
        perror("[client]Eroare la write() spre server.\n");
        return errno;
    }

    system("clear");
    watermark();
    fflush(stdout);

// Apelam functia pentru citirea datelor de autentificare de la tastatura si trimitem catre server
    autentificare(sd);

    system("clear");
    // Primim raspunsul
   watermark();
    if (read(sd, msg, sizeof(msg)) < 0) {
        perror("[client]Eroare la read() de la server.\n");
        return errno;
    }
    printf("\n%s\n\n", msg);
    fflush(stdout);
    sleep(3);
    if (strstr(msg, "succes")) {

        while (strcmp(destinatar, "/q")) {

            system("clear");
            help();
            printf("Lista prieteni:\n\n\n");
            fflush(stdout);

            // Apelam functia pentru citirea listei de prieteni de la server
            lista_prieteni(sd);
            // Citim destinatar de la tastatura
            printf("\nDestinatar: ");
            fflush(stdout);
            bzero(destinatar, sizeof(destinatar));
            scanf("%s", destinatar);
            lungime = strlen(destinatar);
            if (write(sd, & lungime, sizeof(int)) <= 0) {
                perror("[client]Eroare la write() spre server.\n");
                return errno;
            }

            if (write(sd, destinatar, strlen(destinatar)) <= 0) {
                perror("[client]Eroare la write() spre server.\n");
                return errno;
            }

            // Citim raspunsul de la server
            if (strcmp(destinatar, "/q")) {

                while (!strstr(mesaj, "/q")) {

                    system("clear");
                    help();
                    printf("Destinatar: %s\n\n\n", destinatar);
                    fflush(stdout);
                    // Apelam functia pentru citirea si afisarea mesajelor
                    mesaje(sd);
                    printf("\nMesaj: ");
                    fflush(stdout);

                    bzero(mesaj, sizeof(mesaj));
                    // Citim mesajul pentru a-l trimite la server
                    fgets(mesaj, sizeof(mesaj), stdin);
                    fflush(stdin);

                    if (write(sd, mesaj, sizeof(mesaj)) <= 0) {
                        perror("[client]Eroare la write() spre server.\n");
                        return errno;
                    }

                }
                bzero(mesaj, sizeof(mesaj));
            }
        }
        system("clear");
        watermark();
        printf("\nV-ati deconectat.\n\n");
        fflush(stdout);
        sleep(3);system("clear");
    } else system("clear");

    // Inchidem conexiunea, am terminat
    close(sd);
}
