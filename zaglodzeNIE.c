#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long numberofReaders, numberofWriters;

int writersQ = 0;
int readersQ = 0;

pthread_t *writersThreads;
pthread_t *readersThreads;

int writersIn = 0;
int numberOfReadersInReadingRoom = 0;

pthread_cond_t reading = PTHREAD_COND_INITIALIZER;
pthread_cond_t writing = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

int *readersInQueue;
int *writersInQueue;
int *readersInReadingRoom;
int writerInReadingRoom;

void waiting() {
    srand(time(NULL));
    sleep(1 + rand() % (5-1+1));
}

void *writer(void *arg) {
    int ret;
    int nr = *((int*)arg);
    while(1) {
        if((ret = pthread_mutex_lock(&mutex) != 0)) {
            fprintf(stderr, "mutex lock error = %d (&s)\n", ret, sterror(ret));
        }
        if(writersIn || numberOfReadersInReadingRoom > 0) {
            writersQ++;
            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, numberOfReadersInReadingRoom, writersIn);
            if(pthread_cond_wait(&writing, &mutex) == 0) {
                perror("cond_wait error");
            }
            writersQ--;
        }
        writersIn = 1;
        if((ret = pthread_mutex_unlock(&mutex) != 0)) {
            fprintf(stderr, "mutex unlock error = %d (&s)\n", ret, sterror(ret));
        }
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, numberOfReadersInReadingRoom, writersIn);
        
        waiting();

        if((ret = pthread_mutex_lock(&mutex) != 0)) {
            fprintf(stderr, "mutex lock error = %d (&s)\n", ret, sterror(ret));
        }
        writersIn = 0;

        if (readersQ > 0) {
            if(ret = pthread_cond_broadcast(&reading) != 0) {
                fprintf(stderr, "cond_broadcast error = %d (&s)\n", ret, sterror(ret));
            }
        } else {
            if(ret = pthread_cond_broadcast(&writing) != 0) {
                fprintf(stderr, "cond_broadcast error = %d (&s)\n", ret, sterror(ret));
            }
        }
        if((ret = pthread_mutex_unlock(&mutex) != 0)) {
            fprintf(stderr, "mutex unlock error = %d (&s)\n", ret, sterror(ret));
        }
        waiting();
    }
}

void *reader(void *arg) {
    int ret;
    int num = *((int*)arg);
    while(1) {
        if((ret = pthread_mutex_lock(&mutex) != 0)) {
            fprintf(stderr, "mutex lock error = %d (&s)\n", ret, sterror(ret));
        }
        if(writersIn || writersQ > 0) {
            readersQ++;
            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, numberOfReadersInReadingRoom, writersIn);
            if(pthread_cond_wait(&reading, &mutex) == 0) {
                perror("cond_wait error");
            }
            readersQ--;
        }

        numberOfReadersInReadingRoom++;
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, numberOfReadersInReadingRoom, writersIn);
        if(ret = pthread_cond_broadcast(&reading) != 0) {
                fprintf(stderr, "cond_broadcast error = %d (&s)\n", ret, sterror(ret));
            }
        if((ret = pthread_mutex_unlock(&mutex) != 0)) {
            fprintf(stderr, "mutex unlock error = %d (&s)\n", ret, sterror(ret));
        }        
        waiting();
    
        if((ret = pthread_mutex_lock(&mutex) != 0)) {
            fprintf(stderr, "mutex lock error = %d (&s)\n", ret, sterror(ret));
        }
        numberOfReadersInReadingRoom--;
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, numberOfReadersInReadingRoom, writersIn);
        
        if(numberOfReadersInReadingRoom == 0) {
            if(ret = pthread_cond_signal(&reading) != 0) {
                fprintf(stderr, "cond_signal error = %d (&s)\n", ret, sterror(ret));
            }
        }

        if((ret = pthread_mutex_unlock(&mutex) != 0)) {
            fprintf(stderr, "mutex unlock error = %d (&s)\n", ret, sterror(ret));
        }
        waiting();
    }
}

int main(int argc, char *argv[]) {

    if (argc < 3 || argc > 4) {
        printf("Invalid number of arguments!\n");
        exit(EXIT_FAILURE);
    }

    else {
        char *c;
        numberofReaders = strtol(argv[1], &c, 10);
        numberofWriters = strtol(argv[2], &c, 10);
    }

    writersThreads = malloc (sizeof(pthread_t) * numberofWriters);
    readersThreads = malloc (sizeof(pthread_t) * numberofReaders);

    int i, *a, *b;

    for(i = 0; i < numberofWriters; i++) {
        a = (int*)malloc (sizeof(int));
        *a = i;
        if(pthread_create(&writersThreads[i], NULL, &writer, a) != 0) {
            perror("Failed to create thread");
        }
    }

    for(i = 0; i < numberofReaders; i++) {
        b = (int*)malloc (sizeof(int));
        *b = i;
        if(pthread_create(&readersThreads[i], NULL, &reader, b)) {
            perror("Failed to create thread");
        }
    }

    for (i = 0; i < numberofReaders; i++) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Failed to join a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < numberofWriters; i++) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    
    }
    free(readersThreads);
    free(writersThreads);

    exit(EXIT_SUCCESS);
}