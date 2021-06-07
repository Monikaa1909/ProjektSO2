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
sem_t writer;   // możliwość wejścia dla pisarza
sem_t resource;
sem_t tryResource;  //

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
        printf("\nW kolejce: ");
    }

    for (int i = 0; i < numberOfReaders; i++) {
        if (readersInQueue[i] == 1) {
            queue = true;
            numberOfReadersInQueue++;
            if (debug == true) {
                printf("czytelnik nr %d. ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInQueue[i] == 1) {
            queue = true;
            numberOfWritersInQueue++;
            if (debug == true) {
                printf("pisarz nr %d. ", i);
            }
        }
    }
    if (debug == true) {
        if (queue == false) {
            printf("pusto");
        }
        printf("\nW czytelni: ");
    }

    for (int i = 0; i < numberOfReaders; i++) {
        if (readersInReadingRoom[i] == 1) {
            readingRoom = true;
            numberOfReadersInReadingRoom++;
            if (debug == true) {
                printf("czytelnik nr %d. ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInReadingRoom[i] == 1) {
            readingRoom = true;
            numberOfWritersInReadingRoom++;
            if (debug == true) {
                printf("pisarz nr %d. ", i);
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
        sem_wait(&tryResource);

        // uzyskał możliwość wejścia, wychodzi z kolejki, wchodzi do środka:
        sem_wait(&reader);
        numberOfReadersInReadingRoom++;
        if (numberOfReadersInReadingRoom == 1) {  // jeśli jest pierwszym czytelnikiem w czytelni, zablokuj możliwość dostępu do zasobów czytelni
            sem_wait(&resource);
        }

        readersInQueue[nr] = 0;
        readersInReadingRoom[nr] = 1;
        printf("\n(wejście czytelnika nr %d do środka)\n", nr);

        whoIsWhere();

        sem_post(&reader);
        sem_post(&tryResource); // inni czytelnicy mogą próbować wchodzić do środka, skoro w środku aktualnie jest czytelnik

        // korzysta z biblioteki:
        waiting();

        // wychodzi ze środka:
        sem_wait(&reader);
        numberOfReadersInReadingRoom--;
        readersInReadingRoom[nr] = 0;
        if (numberOfReadersInReadingRoom == 0) {  // jeśli był ostatnim czytelnikiem w czytelni, umożliwia dostęp do zasobów czytelni
            sem_post(&resource);
        }
        sem_post(&reader);

        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    return 0;
}

void *writer_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        // pisarz wchodzi do kolejki:
        sem_wait(&writer);
        numberOfWaitingWriters++;
        if (numberOfWaitingWriters == 1) {   // jeżeli jest pierwszym pisarzem w kolejce, blokuje możliwość próby wejścia do czytelni dla czytelników:
            sem_wait(&tryResource);
        }
        printf("\n(wejście pisarza nr %d do kolejki alert) \n", nr);
        writersInQueue[nr] = 1;
        sem_post(&writer);

        //  czeka na dostęp do zasobów czytelni, wychodzi z kolejki, wchodzi do środka i blokuje innym pisarzom możliwość wejścia:
        sem_wait(&resource);
        writersInQueue[nr] = 0;
        writersInReadingRoom[nr] = 1;
        printf("\n(wejście pisarza nr %d do środka) \n", nr);
        whoIsWhere();
        // korzysta z biblioteki:
        waiting();

        // wychodzi, zwalniając dostęp do czytelni:
        sem_post(&resource);
        sem_wait(&writer);
        numberOfWaitingWriters--;
        if (numberOfWaitingWriters == 0) {   // jeżeli nie ma już czekających pisarzy, czytelnicy mogą próbować wejść:
            sem_post(&tryResource);
        }
        writersInReadingRoom[nr] = 0;
        sem_post(&writer);

        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    return 0;
}

void init() {
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
    if (argc < 3 || argc > 4) {
        printf("Invalid number of arguments!\n");
        exit(EXIT_FAILURE);
    }

    else {
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

    readersThreads = malloc(sizeof(pthread_t) * numberOfReaders);
    writersThreads = malloc(sizeof(pthread_t) * numberOfWriters);

    init();

    int i, *a, *b;
    for (i = 0; i < numberOfReaders; i++) {
        a = (int*)malloc (sizeof(int));
        *a = i;
        if (pthread_create(&readersThreads[i], NULL, &reader_, a) != 0) {
            perror("Failed to create a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < numberOfWriters; i++) {
        b = (int*)malloc (sizeof(int));
        *b = i;
        if (pthread_create(&writersThreads[i], NULL, &writer_, b) != 0) {
            perror("Failed to create a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < numberOfReaders; i++) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Failed to join a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < numberOfWriters; i++) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    }

    //free(readersThreads);
    //free(writersThreads);
    
    pthread_exit(EXIT_SUCCESS);
} 
