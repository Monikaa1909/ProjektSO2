#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>

long readers;
long writers;
bool debug = false;

pthread_t *readersThreads;  // tablica wątków czytelników
pthread_t *writersThreads;  // tablica wątków pisarzy

sem_t writing;  // daje znać, czy jest wolne miejsce dla jednego pisarza w środku (1 - jest, 0 - nie ma)
sem_t reading;	// daje znać, czy jest wolne miiejsce dla dowolnej ilości czytelników w środku (1 - jest, 0 - nie ma)

int readersQ;    // czytelnicy w kolejce
int writersQ;    // pisarze w kolejce
int readersIn;   // czytelnicy w środku
int writersIn;   // pisarze w środku

void *reader (){
    printf("Tu reader\n");
    pthread_exit(0);
}

void *writer (){
    printf("Tu writer\n");

//    while (1) {
        readersQ++;  // mamy czytelnika w kolejce
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);
//    }

    pthread_exit(0);
}

int main (int argc, char *argv[]){
    if (argc < 3 || argc > 4) {
//        błąd w podawaniu argumentów
    }

    else {
        char *c;
        readers = strtol(argv[1], &c, 10);
        writers = strtol(argv[2], &c, 10);
        if (argc == 4) {
            if (strcmp(argv[3], "-debug") != 0) {
//                błąd przy ostatnim argumencie
            }
            else debug = true;
        }
    }

    readersThreads = malloc (sizeof(pthread_t) * readers);
    writersThreads = malloc (sizeof(pthread_t) * writers);

//    na początku w środku jest miejsce dla wszystkich, bo nie wiemy kto dokładnie jest w kolejce:
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
        };
    }

//    for (int i = 0; i < writers; ++i) {
//        pthread_join(writersThreads[i], NULL);
//    }
//
//    for (int i = 0; i < readers; ++i) {
//        pthread_join(readersThreads[i], NULL);
//    }

    pthread_exit(0);
}