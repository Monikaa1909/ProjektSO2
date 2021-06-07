#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long readers, writers;

int writersQ = 0;
int readersQ = 0;

pthread_t *writersThreads;
pthread_t *readersThreads;

int writersIn = 0;
int readersIn = 0;

pthread_cond_t reading = PTHREAD_COND_INITIALIZER;
pthread_cond_t writing = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

void waiting() {
    srand(time(NULL));
    sleep(1 + rand() % (5-1+1));
}

void *writer(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        pthread_mutex_lock(&mutex);
        if(writersIn || readersIn > 0) {
            writersQ++;
            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
            pthread_cond_wait(&writing, &mutex);
            writersQ--;
        }
        writersIn = 1;
        pthread_mutex_unlock(&mutex);
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        
        waiting();

        pthread_mutex_lock(&mutex);
        writersIn = 0;

        if (readersQ > 0) {
            pthread_cond_broadcast(&reading);
        } else {
            pthread_cond_broadcast(&writing);
        }
        pthread_mutex_unlock(&mutex);
        waiting();
    }
}

void *reader(void *arg) {
    int num = *((int*)arg);
    while(1) {
        pthread_mutex_lock(&mutex);
        if(writersIn || writersQ > 0) {
            readersQ++;
            printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
            pthread_cond_wait(&reading, &mutex);
            readersQ--;
        }

        readersIn++;
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        pthread_cond_broadcast(&reading);
        pthread_mutex_unlock(&mutex);
        
        waiting();
    
        pthread_mutex_lock(&mutex);
        readersIn--;
        printf("ReaderQ: %d WriterQ: %d [in: R: %d W: %d]\n", readersQ, writersQ, readersIn, writersIn);
        
        if(readersIn == 0) {
            pthread_cond_signal(&writing);
        }

        pthread_mutex_unlock(&mutex);
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
        readers = strtol(argv[1], &c, 10);
        writers = strtol(argv[2], &c, 10);
    }

    writersThreads = malloc (sizeof(pthread_t) * writers);
    readersThreads = malloc (sizeof(pthread_t) * readers);

    int i, *a, *b;

    for(i = 0; i < writers; i++) {
        a = (int*)malloc (sizeof(int));
        *a = i;
        if(pthread_create(&writersThreads[i], NULL, &writer, a) != 0) {
            perror("Failed to create thread");
        }
    }

    for(i = 0; i < readers; i++) {
        b = (int*)malloc (sizeof(int));
        *b = i;
        if(pthread_create(&readersThreads[i], NULL, &reader, b)) {
            perror("Failed to create thread");
        }
    }

    for (i = 0; i < readers; i++) {
        if (pthread_join(readersThreads[i], NULL) != 0) {
            perror("Failed to join a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < writers; i++) {
        if (pthread_join(writersThreads[i], NULL) != 0) {
            perror("Failed to join threads");
            exit(EXIT_FAILURE);
        }
    
    }
    free(readersThreads);
    free(writersThreads);

    exit(EXIT_SUCCESS);
}