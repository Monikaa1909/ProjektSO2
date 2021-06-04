#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>

long readers;
long writers;
bool debug = false;

pthread_t *readersThreads;  // tablica wątków czytelników
pthread_t *writersThreads;  // tablica wątków pisarzy

sem_t writing;  // daje znać, czy jest wolne miejsce dla jednego pisarza w środku
sem_t reading;	// daje znać, czy jest wolne miiejsce dla dowolnej ilości czytelników w środku

int readersQ;    // czytelnicy w kolejce
int writersQ;    // pisarze w kolejce
int readersIn;   // czytelnicy w środku
int writersIn;   // pisarze w środku

int czytelnik = 1;
int pisarz = 1;

void *reader (){

    while(1) {
        int nr = czytelnik;
        czytelnik++;
        readersQ++; // czytelnik wchodzi do kolejki
        printf("(wejście czytelnika nr %d do kolejki) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);

        // czeka na możliwość wejścia, wychodzi z kolejki, wchodzi do środka:
        sem_wait(&reading);

        if (readersIn == 0) {   // jeżeli będzie pierwszym czytelnikiem, blokuje pisarzom możliwość pisania:
            sem_wait(&writing);
        }

        readersQ--;
        readersIn++;
        printf("(wejście czytelnika nr %d do środka) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);
        sem_post(&reading);

        // korzysta z biblioteki:
        sleep(2);

        // wychodzi ze środka:
//        sem_wait(&reading);
        readersIn--;
        printf("(wyjście czytelnika nr %d ze środka) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);
        if (readersIn == 0) {   // jeżeli był ostatnim czytelnikiem, odblokowuje pisarzom możliwość pisania:
            sem_post(&writing);
        }
//        sem_post(&reading);

        // czas, po którym czytelnik wróci do kolejki:
        sleep(4);
    }
    return 0;
}

void *writer (){

    while (1) {
        int nr = pisarz;
        pisarz++;
        writersQ++;  // pisarz wchodzi do kolejki
        printf("(wejście pisarza nr %d do kolejki) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);

        //  na możliwość wejścia, wychodzi z kolejki, wchodzi do środka i blokuje pozostałym pisarzom możliwość pisania:
        sem_wait(&writing);
        writersQ--;
        writersIn++;
        printf("(wejście pisarza nr %d do środka) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);

        // pisarz korzysta z biblioteki:
        sleep(2);

        // wychodzi ze środka i odblokowuje dostęp dla pisarzy:
        writersIn--;
        printf("(wyjście pisarza nr %d ze środka) ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", nr, readersQ, writersQ, readersIn, writersIn);
        sem_post(&writing);

        // czas, po którym pisarz wróci do kolejki:
        sleep(4);
    }
    return 0;
}

int main (int argc, char *argv[]){
    if (argc < 3 || argc > 4) {
        perror("Wrong arguments");
        exit(EXIT_FAILURE);
    }

    else {
        char *c;
        readers = strtol(argv[1], &c, 10);
        writers = strtol(argv[2], &c, 10);
        if (argc == 4) {
            if (strcmp(argv[3], "-debug") != 0) {
                perror("Wrong arguments");
                exit(EXIT_FAILURE);
            }
            else debug = true;
        }
    }

    // inizjalizacja tablic wątków
    if ((readersThreads = malloc (sizeof(pthread_t) * readers)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    if ((writersThreads = malloc (sizeof(pthread_t) * writers)) == NULL) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    // inicjalizacja semaforów
    if (sem_init(&reading, 0, 1) != 0) {
        perror("Sem_init error");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&writing, 0, 1) != 0) {
        perror("Sem_init error");
        exit(EXIT_FAILURE);
    }

//    stworzenie wątku dla każdego czytelnika i pisarza:
    int err;
    for (int i = 0; i < readers; i++) {
        if ((err = pthread_create( &readersThreads[i], NULL, &reader, NULL)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < writers; i++) {
        if ((err = pthread_create( &writersThreads[i], NULL, &writer, NULL)) != 0) {
            fprintf (stderr, "Thread creation error = %d (%s)\n", err, strerror (err));
            exit(EXIT_FAILURE);
        }
    }

    // oczzekiwanie na zakończenie wątków czytelników i pisarzy:
    for (int i = 0; i < readers; ++i) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Pthread_join error");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < writers; ++i) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Pthread_join error");
            exit(EXIT_FAILURE);
        }
    }

    free(readersThreads);
    free(writersThreads);
    sem_destroy(&reading);
    sem_destroy(&writing);

    pthread_exit(0);
}