#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

long numberOfReaders;   // ilość wszystkich czytelników
long numberOfWriters;   // ilość wszystkich pisarzy
bool debug = false; // opcja wypisująca dokładną zawartość kolejki i czytelni

pthread_t *readersThreads;  // tablica wątków czytelników
pthread_t *writersThreads;  // tablica wątków pisarzy

sem_t writing;  // daje znać, czy jest wolne miejsce dla jednego pisarza w środku
sem_t reading;	// daje znać, czy jest wolne miiejsce dla dowolnej ilości czytelników w środku

int numberOfUsingResources = 0;

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
                printf("czytelnik nr %d ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInQueue[i] == 1) {
            queue = true;
            numberOfWritersInQueue++;
            if (debug == true) {
                printf("pisarz nr %d ", i);
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
                printf("czytelnik nr %d ", i);
            }
        }
    }
    for (int i = 0; i < numberOfWriters; i++) {
        if (writersInReadingRoom[i] == 1) {
            readingRoom = true;
            numberOfWritersInReadingRoom++;
            if (debug == true) {
                printf("pisarz nr %d ", i);
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

void *reader (void *arg){
    int nr = *((int*)arg);
    while(1) {
        // czytelnik wchodzi do kolejki
        readersInQueue[nr] = 1;

        // czeka na możliwość wejścia, wychodzi z kolejki, wchodzi do środka:
        if (sem_wait(&reading) == 1) {
            perror("sem_wait");
        }
        numberOfUsingResources++;
        if (numberOfUsingResources == 1) {   // jeżeli będzie pierwszym czytelnikiem, blokuje pisarzom możliwość pisania:
            if(sem_post(&writing) == 1) {
            perror("sem_post");
        }
        }
        readersInQueue[nr] = 0;
        readersInReadingRoom[nr] = 1;
        printf("\n(wejście czytelnika nr %d do środka)\n", nr);

        whoIsWhere();

        if(sem_post(&reading) == 1) {
            perror("sem_post");
        }

        // korzysta z biblioteki:
        waiting();

        // wychodzi ze środka:
        if (sem_wait(&reading) == 1) {
            perror("sem_wait");
        }
        numberOfUsingResources--;
        if (numberOfUsingResources == 0) {   // jeżeli był ostatnim czytelnikiem, odblokowuje pisarzom możliwość pisania:
            if(sem_post(&writing) == 1) {
            perror("sem_post");
        }
        }
        readersInReadingRoom[nr] = 0;
        if(sem_post(&reading) == 1) {
            perror("sem_post");
        }

        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    pthread_exit(0);
}

void *writer (void *arg){
    int nr = *((int*)arg);
    while (1) {
        // pisarz wchodzi do kolejki:
        writersInQueue[nr] = 1;

        //  ma możliwość wejścia, wychodzi z kolejki, wchodzi do środka i blokuje pozostałym pisarzom możliwość pisania:
        if (sem_wait(&writing) == 1) {
            perror("sem_wait");
        }
        writersInQueue[nr] = 0;
        writersInReadingRoom[nr] = 1;
        printf("\n(wejście pisarza nr %d do środka) \n", nr);

        whoIsWhere();

        // pisarz korzysta z biblioteki:
        waiting();

        // wychodzi ze środka i odblokowuje dostęp dla pisarzy:
        writersInReadingRoom[nr] = 0;
        if(sem_post(&writing) == 1) {
            perror("sem_post");
        }

        // czas, po którym pisarz wróci do kolejki:
        waiting();
    }
    pthread_exit(0);
}

int main (int argc, char *argv[]){
    // wczytanie ilości czytelników i pisarzy oraz ewentualnie opcji -debug:
    if (argc < 3 || argc > 4) {
        perror("Wrong arguments");
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
    if (sem_init(&reading, 0, 1) != 0) {
        perror("Sem_init error");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&writing, 0, 1) != 0) {
        perror("Sem_init error");
        exit(EXIT_FAILURE);
    }

    printf("Program wypisuje aktualny stan kolejki i czytelni za każdym razem, gdy kolejna osoba wejdzie do środka.\n");

    //    stworzenie wątku dla każdego czytelnika i pisarza:
    int *nr, err;
    for (int i = 0; i < numberOfReaders; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if ((err = pthread_create(&readersThreads[i], NULL, &reader, nr)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfWriters; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if ((err = pthread_create(&writersThreads[i], NULL, &writer, nr)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    // oczekiwanie na zakończenie wątków czytelników i pisarzy:
    for (int i = 0; i < numberOfReaders; ++i) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Pthread_join error");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfWriters; ++i) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Pthread_join error");
            exit(EXIT_FAILURE);
        }
    }

    free(readersThreads);
    free(writersThreads);
    free(readersInQueue);
    free(writersInQueue);
    free(readersInReadingRoom);
    free(writersInReadingRoom);
    sem_destroy(&reading);
    sem_destroy(&writing);

    pthread_exit(0);
}