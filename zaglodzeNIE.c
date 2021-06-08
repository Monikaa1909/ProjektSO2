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

pthread_t *writersThreads;
pthread_t *readersThreads;

int numberOfWritersInQueue_ = 0;
int numberOfReadersInQueue_ = 0;
int numberOfWritersInReadingRoom_ = 0;
int numberOfReadersInReadingRoom_ = 0;

int *readersInQueue = 0;    // tablica czytelników w kolejce
int *writersInQueue = 0;    // tablica pisarzy w kolejce
int *readersInReadingRoom = 0;  // tablica czytelników w czytelni
int *writersInReadingRoom = 0;  // tablica pisarzy w czytelni

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

        // jeśli ktoś jest w czytelni:
        if(numberOfWritersInReadingRoom_ || numberOfReadersInReadingRoom_ > 0) {
            // pisarz wchodzi do kolejki
            printf("\n(wejście pisarza nr %d do kolejki)\n", nr);
            numberOfWritersInQueue_++;
            writersInQueue[nr] = 1;

            // czeka na możliwość wejścia i wychodzi z kolejki:
            pthread_cond_wait(&writer, &mutex);
            numberOfWritersInQueue_--;
            writersInQueue[nr] = 0;
        }

        // wchodzi do środka:
        numberOfWritersInReadingRoom_ = 1;
        writersInReadingRoom[nr] = 1;
        pthread_mutex_unlock(&mutex);
        printf("\n(wejście pisarza nr %d do środka)", nr);
        whoIsWhere();

        // korzysta z biblioteki:
        waiting();

        // wychodzi ze środka:
        pthread_mutex_lock(&mutex);
        numberOfWritersInReadingRoom_ = 0;
        writersInReadingRoom[nr] = 0;

        // jeśli w kolejce są jacyś czytelnicy, wpuszcza ich:
        if (numberOfReadersInQueue_ > 0) {
            pthread_cond_broadcast(&reader);
        }

        // jeśli nie, wpuszcza pierwszego z kolejki pisarza:
        else {
            pthread_cond_broadcast(&writer);
        }
        pthread_mutex_unlock(&mutex);

        // czas, po którym czytelnik wróci do kolejki:
        waiting();
    }
    pthread_exit(0);
}

void *reader_(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        pthread_mutex_lock(&mutex);
        // jeśli w czytelni jest pisarz lub jakiś pisarz czeka w kolejce:
        if(numberOfWritersInReadingRoom_ || numberOfWritersInQueue_ > 0) {
            // czytelnik wchodzi do kolejki:
            numberOfReadersInQueue_++;
            readersInQueue[nr] = 1;
            printf("\n(wejście czytelnika nr %d do kolejki)\n", nr);

            // czeka na możliwość czytania i wychodzi z kolejki:
            pthread_cond_wait(&reader, &mutex);
            numberOfReadersInQueue_--;
            readersInQueue[nr] = 0;
        }

        // wchodzi do środka:
        numberOfReadersInReadingRoom_++;
        readersInReadingRoom[nr] = 1;
        printf("\n(wejście czytelnika nr %d do środka)", nr);
        whoIsWhere();

        // pozwala na wejście dla pozostałych czytelników z kolejki:
        pthread_cond_broadcast(&reader);
        pthread_mutex_unlock(&mutex);

        // korzysta z biblioteki:
        waiting();

        // wychodzi ze środka:
        pthread_mutex_lock(&mutex);
        numberOfReadersInReadingRoom_--;
        readersInReadingRoom[nr] = 0;

        // jeśli był ostatnim czytelnikiem w środku, wpuszcza pisarza, który jest pierwszy w kolejce:
        if(numberOfReadersInReadingRoom_ == 0) {
            pthread_cond_signal(&writer);
        }

        pthread_mutex_unlock(&mutex);
        waiting();
    }
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
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

    printf("Program wypisuje aktualny stan kolejki i czytelni za każdym razem, gdy kolejna osoba wejdzie do środka.\n");

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
    free(readersInQueue);
    free(writersInQueue);
    free(readersInReadingRoom);
    free(writersInReadingRoom);

    exit(EXIT_SUCCESS);
}