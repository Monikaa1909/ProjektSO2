#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

long numberOfReaders;
long numberOfWriters;
bool debug = false;

int writersQ = 0;
int readersQ = 0;

pthread_t *writersThreads;
pthread_t *readersThreads;

int writersIn = 0;
int readersIn = 0;

int *readersInQueue = 0;    // tablica czytelników w kolejce
int *writersInQueue = 0;    // tablica pisarzy w kolejce
int *readersInReadingRoom = 0;  // tablica czytelników w czytelni
int *writersInReadingRoom = 0;  // tablica pisarzy w czytelni


pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexInit = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t reader = PTHREAD_COND_INITIALIZER;   // zmienna warunkowa pozwalająca na czytanie dla czytelnika
pthread_cond_t writer = PTHREAD_COND_INITIALIZER;  // zmienna warunkowa pozwalająca na pisanie dla pisarza
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

void waiting() {
    srand(time(NULL));
    sleep(1 + rand() % (5-1+1));
}

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

void init(){
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

void *writer_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        pthread_mutex_lock(&mutex);
        if(writersIn || readersIn > 0) {
            writersQ++;
            writersInQueue[nr] = 1;
//            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
            pthread_cond_wait(&writer, &mutex);
            writersQ--;
            writersInQueue[nr] = 0;
        }
        writersIn = 1;
        writersInReadingRoom[nr] = 1;
        pthread_mutex_unlock(&mutex);
        printf("\n(wejście pisarza nr %d do środka)", nr);
//        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        whoIsWhere();
        waiting();

        pthread_mutex_lock(&mutex);
        writersIn = 0;
        writersInReadingRoom[nr] = 0;

        if (readersQ > 0) {
            pthread_cond_broadcast(&reader);
        } else {
            pthread_cond_broadcast(&writer);
        }
        pthread_mutex_unlock(&mutex);
        waiting();
    }
}

void *reader_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        pthread_mutex_lock(&mutex);
        if(writersIn || writersQ > 0) {
            readersQ++;
            readersInQueue[nr] = 1;
//            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
            pthread_cond_wait(&reader, &mutex);
            readersQ--;
            readersInQueue[nr] = 0;
        }

        readersIn++;
        readersInReadingRoom[nr] = 1;
        printf("\n(wejście czytelnika nr %d do środka)", nr);
        whoIsWhere();
//        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        pthread_cond_broadcast(&reader);
        pthread_mutex_unlock(&mutex);
        
        waiting();
    
        pthread_mutex_lock(&mutex);
        readersIn--;
        readersInReadingRoom[nr] = 0;
//        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        
        if(readersIn == 0) {
            pthread_cond_signal(&writer);
        }

        pthread_mutex_unlock(&mutex);
        waiting();
    }
}

int main(int argc, char *argv[]) {
// wczytanie ilości czytelników i pisarzy oraz ewentualnie opcji -debug:
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

    init();

    // stworzenie wątku dla każdego czytelnika i pisarza:
    int *nr;
    for (int i = 0; i < numberOfReaders; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if (pthread_create(&readersThreads[i], NULL, &reader_, nr) != 0) {
            perror("Failed to create a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberOfWriters; i++) {
        nr = (int*)malloc (sizeof(int));
        *nr = i;
        if (pthread_create(&writersThreads[i], NULL, &writer_, nr) != 0) {
            perror("Failed to create a thread");
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

    exit(EXIT_SUCCESS);
}