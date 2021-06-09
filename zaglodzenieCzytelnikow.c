#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>

long numberOfReaders;
long numberOfWriters;
bool debug = false;

sem_t reader;
sem_t writer;
sem_t resource; // dostęp do czytelni
sem_t tryResource;  //  czy warto w ogóle próbować dostać się do czytelni

pthread_t *readersThreads;
pthread_t *writersThreads;

int numberOfWaitingWriters = 0; // pisarze chcący dostępu do czytelni
int numberOfReadersInReadingRoom = 0; // czytelnicy aktualnie w czytelni

int *readersInQueue = 0;    // tablica czytelników w kolejce
int *writersInQueue = 0;    // tablica pisarzy w kolejce
int *readersInReadingRoom = 0;  // tablica czytelników w czytelni
int *writersInReadingRoom = 0;  // tablica pisarzy w czytelni

void whoIsWhere() {
    int numberOfReadersInQueue = 0;    // ilość czytelników w kolejce
    int numberOfWritersInQueue = 0;    // ilość pisarzy w kolejce
    int numberOfReadersInReadingRoom = 0;   // ilość czytelników w środku
    int numberOfWritersInReadingRoom = 0;   // ilość pisarzy w środku
    bool queue = false; // czy ktokolwiek w kolejce
    bool readingRoom = false;   // czy ktokolwiek w czytelni

    if (debug == true) {
        printf("\nW KOLEJCE: ");
    }

    for (int i = 0; i < numberOfReaders; i++) {
        if (readersInQueue[i] == 1) {
            queue = true;
            numberOfReadersInQueue++;
            if (debug == true) {
                printf("czytelnik nr%d ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInQueue[i] == 1) {
            queue = true;
            numberOfWritersInQueue++;
            if (debug == true) {
                printf("pisarz nr%d ", i);
            }
        }
    }
    if (debug == true) {
        if (queue == false) {
            printf("pusto");
        }
        printf("\nW CZYTELNI: ");
    }

    for (int i = 0; i < numberOfReaders; i++) {
        if (readersInReadingRoom[i] == 1) {
            readingRoom = true;
            numberOfReadersInReadingRoom++;
            if (debug == true) {
                printf("czytelnik nr%d ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInReadingRoom[i] == 1) {
            readingRoom = true;
            numberOfWritersInReadingRoom++;
            if (debug == true) {
                printf("pisarz nr%d ", i);
            }
        }
    }
    if (debug == true) {
        if (readingRoom == false) {
            printf("pusto\n");
        }
    }

    printf("\nReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", numberOfReadersInQueue, numberOfWritersInQueue, numberOfReadersInReadingRoom, numberOfWritersInReadingRoom);
}

void waiting() {
    srand(time(NULL));
    sleep(1 + rand() % (5-1+1));
}

void *reader_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        // czytelnik wchodzi do kolejki
        readersInQueue[nr] = 1;

        // czytelnik czeka na możliwość wejścia do środka:
        if(sem_wait(&tryResource) == 1) {
            perror("sem_wait");
        }

        // uzyskał możliwość wejścia, wychodzi z kolejki, wchodzi do środka:
        if(sem_wait(&reader) == 1) {
            perror("sem_wait");
        }
        numberOfReadersInReadingRoom++;
        if (numberOfReadersInReadingRoom == 1) {  // jeśli jest pierwszym czytelnikiem w czytelni, zablokuj możliwość dostępu do zasobów czytelni
            if(sem_wait(&resource) == 1) {
                perror("sem_wait");
            }
        }

        readersInQueue[nr] = 0;
        readersInReadingRoom[nr] = 1;
        printf("\n(wejście czytelnika nr %d do środka)", nr);

        whoIsWhere();

        if(sem_post(&reader) == 1) {
            perror("sem_wait");
        }
        if(sem_post(&tryResource) == 1) {
            perror("sem_post");
        } // inni czytelnicy mogą próbować wchodzić do środka, skoro w środku aktualnie jest czytelnik

        // korzysta z biblioteki:
        waiting();

        // wychodzi ze środka:
        if(sem_wait(&reader) == 1) {
            perror("sem_wait");
        }

        numberOfReadersInReadingRoom--;
        readersInReadingRoom[nr] = 0;
        if (numberOfReadersInReadingRoom == 0) {  // jeśli był ostatnim czytelnikiem w czytelni, umożliwia dostęp do zasobów czytelni
            sem_post(&resource);
        }
        if(sem_post(&reader) == 1) {
            perror("sem_post");
        }


        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    pthread_exit(0);
}

void *writer_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        // pisarz wchodzi do kolejki:
        if(sem_wait(&writer) == 1) {
            perror("sem_wait");
        }
        numberOfWaitingWriters++;
        if (numberOfWaitingWriters == 1) {   // jeżeli jest pierwszym pisarzem w kolejce, blokuje możliwość próby wejścia do czytelni dla czytelników:
            if(sem_wait(&tryResource) == 1) {
                perror("sem_wait");
            }
        }
        printf("\n(wejście pisarza nr %d do kolejki!)\n", nr);
        writersInQueue[nr] = 1;
        if(sem_post(&writer) == 1) {
            perror("sem_post");
        }

        //  czeka na dostęp do zasobów czytelni, wychodzi z kolejki, wchodzi do środka i blokuje innym pisarzom możliwość wejścia:
        if(sem_wait(&resource)) {
            perror("sem_wait");
        }
        writersInQueue[nr] = 0;
        writersInReadingRoom[nr] = 1;
        printf("\n(wejście pisarza nr %d do środka)", nr);
        whoIsWhere();

        // korzysta z biblioteki:
        waiting();

        // wychodzi, zwalniając dostęp do czytelni:
        if(sem_post(&resource) == 1) {
            perror("sem_post");
        }
        if (sem_wait(&writer) == 1) {
            perror("sem_wait");
        }
        numberOfWaitingWriters--;
        if (numberOfWaitingWriters == 0) {   // jeżeli nie ma już czekających pisarzy, czytelnicy mogą próbować wejść:
            if(sem_post(&tryResource) == 1) {
                perror("sem_post");
            }
        }
        writersInReadingRoom[nr] = 0;
        if(sem_post(&writer) == 1) {
            perror("sem_post");
        }

        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    pthread_exit(0);
}

void init() {
    // inizjalizacja tablic wątków
    if ((readersThreads = malloc (sizeof(pthread_t) * numberOfReaders)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    if ((writersThreads = malloc (sizeof(pthread_t) * numberOfWriters)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    // inicjalizacja tablic przechowująca czytelników i pisarzy w kolejce i w czytelni:
    if ((writersInQueue = malloc (sizeof(int) * numberOfWriters)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    if ((writersInReadingRoom = malloc (sizeof(int) * numberOfWriters)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numberOfWriters; i++) {
        writersInReadingRoom[i] = 0;
        writersInQueue[i] = 0;
    }

    if ((readersInQueue = malloc (sizeof(int) * numberOfReaders)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    if ((readersInReadingRoom = malloc (sizeof(int) *numberOfReaders)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numberOfReaders; i++) {
        readersInReadingRoom[i] = 0;
        readersInQueue[i] = 0;
    }
}


int main (int argc, char *argv[]) {
    // wczytanie ilości czytelników i pisarzy oraz ewentualnie opcji -debug:
    if (argc < 3 || argc > 4) {
        printf("Invalid number of arguments!\n");
        exit(EXIT_FAILURE);
    } else {
        char *c;
        numberOfReaders = strtol(argv[1], &c, 10);
        numberOfWriters = strtol(argv[2], &c, 10);
        if (argc == 4) {
            if (strcmp(argv[3], "-debug") != 0) {
                perror("Wrong arguments");
                exit(EXIT_FAILURE);
            }
            else debug = true;
        }
    }

    init();

    // inicjalizacja semaforów:
    if (sem_init(&reader, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&writer, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&resource, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&tryResource, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }

    printf("Program wypisuje aktualny stan kolejki i czytelni za każdym razem, gdy kolejna osoba wejdzie do środka.\n");

    // stworzenie wątku dla każdego czytelnika i pisarza:
    int *nr, err;
    for (int i = 0; i < numberOfReaders; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if ((err = pthread_create(&readersThreads[i], NULL, &reader_, nr)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfWriters; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if ((err = pthread_create(&writersThreads[i], NULL, &writer_, nr)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfReaders; i++) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Failed to join a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfWriters; i++) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    }

    free(readersThreads);
    free(writersThreads);
    free(readersInQueue);
    free(writersInQueue);
    free(readersInReadingRoom);
    free(writersInReadingRoom);
    sem_destroy(&reader);
    sem_destroy(&writer);
    sem_destroy(&tryResource);
    sem_destroy(&resource);

    pthread_exit(0);
}