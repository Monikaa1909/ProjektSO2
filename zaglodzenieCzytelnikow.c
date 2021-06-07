#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>

long readers, writers;
bool debug = false;

int readersIn = 0;
int writersIn = 0;

sem_t reading, writing, temp1, temp2, temp3;

pthread_t *readersThreads;
pthread_t *writersThreads;

int readersQ = 0;
int writersQ = 0;  

void *reader(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        readersQ++;
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        sem_wait(&temp3);         
        sem_wait(&reading);
        sem_wait(&temp1);          

        readersQ--;
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        readersIn++;                //powiadomienie o wejsciu czytelnika
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        if (readersIn == 1) {   
            sem_wait(&writing);    //jezeli jest pierwszym czytajacym zablokuj zasob 
        }
        sem_post(&temp1);         
        sem_post(&reading);
        sem_post(&temp3);        
        
        printf("Reader: %d reading..\n", nr);
        sleep(2);
        sem_wait(&temp1);
        readersIn--;
        printf("Reader: %d left.\n", nr);
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        if (readersIn == 0) {
            sem_post(&writing);
        }
        sem_post(&temp1);
        sleep(3);
    }
    free(arg); 
}

void *writer(void *arg) {
    int nr = *((int*)arg);
    while(1) {
        writersQ++;
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        sem_wait(&temp2);
        writersQ--;
        writersIn = 1;
        printf("Writer: %d entered.\n", nr);
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersQ, readersIn, writersIn);

        if (writersQ == 1) {
            sem_wait(&reading);
        }

        sem_post(&temp2);
        sem_wait(&writing);

            // CRITICAL SECTION

        printf("Writer: %d writing..\n", nr);
        sleep(5);

        writersIn = 0;
        sem_post(&writing);
        sem_wait(&temp2);
        
        printf("Writer: %d left.\n", nr);
        printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersQ, writersIn, readersIn, writersQ);

        if (writersQ == 0) {
            sem_post(&reading);
        }
        sem_post(&temp2);
        sleep(5);
    }
    free(arg);
}

void init() {
    if (sem_init(&reading, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&writing, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&temp1, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&temp2, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&temp3, 0, 1) != 0) {
        perror("Sem init error!");
        exit(EXIT_FAILURE);
    }
}


int main (int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        printf("Invalid number of arguments!\n");
        exit(EXIT_FAILURE);
    }

    else {
        char *c;
        readers = strtol(argv[1], &c, 10);
        writers = strtol(argv[2], &c, 10);
    }

    readersThreads = malloc(sizeof(pthread_t) * readers);
    writersThreads = malloc(sizeof(pthread_t) * writers);

    init();

    int i, *a, *b;
    for (i = 0; i < readers; i++) {
        a = (int*)malloc (sizeof(int));
        *a = i;
        if (pthread_create(&readersThreads[i], NULL, &reader, a) != 0) {
            perror("Failed to create a thread");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < writers; i++) {
        b = (int*)malloc (sizeof(int));
        *b = i;
        if (pthread_create(&writersThreads[i], NULL, &writer, b) != 0) {
            perror("Failed to create a thread");
            exit(EXIT_FAILURE);
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

    //free(readersThreads);
    //free(writersThreads);
    
    pthread_exit(EXIT_SUCCESS);
} 
