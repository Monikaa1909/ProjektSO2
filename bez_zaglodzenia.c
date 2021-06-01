//rozwiazanie bez zaglodzenia ale bez uzycia semafor�w !!
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>


int numberOfWriters;
int numberOfReaders;
int writersInQueue = 0;
int readersInQueue = 0;
int debugMode = 0; //1 - on, 0 - off
pthread_t* writers; 
pthread_t* readers; 
int reading = 0;
int writing = 0;
pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexInit = PTHREAD_MUTEX_INITIALIZER;



void* writer (void* arg) {
	int num = *((int*)arg);
	while(1) {
		pthread_mutex_lock(&mutexInit);
		//jezeli jest juz pisarz lub jakis czytelnik
		if (writing || reading > 0) {	
			writersInQueue++;
			printf("5	ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, writersInQueue, reading, writing);
			pthread_cond_wait(&canWrite, &mutexInit); //oczekuje na komunikat ze mozna zaczac pisanie
			writersInQueue--;
		}
		
		writing = 1;
		pthread_mutex_unlock(&mutexInit);
		printf("7	ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, writersInQueue, reading, writing);
		
		sleep(2);
		
		pthread_mutex_lock(&mutexInit);
		writing = 0;
		
		
		if (readersInQueue > 0) { 
			pthread_cond_broadcast(&canRead); //komnunikuje sie z czytelnikami w kolejce
		} else { 
			pthread_cond_broadcast(&canWrite); //komunikuje sie z pisarzami w kolejce
		}
		
		pthread_mutex_unlock(&mutexInit);
		sleep(2);
	}
}


void* reader (void* arg) {
	int num = *((int*)arg);
	while(1) {
		pthread_mutex_lock(&mutexInit);
		//jezeli juz ktos pisze albo jakis pisarz czeka
		if (writing || writersInQueue > 0) {
			readersInQueue++;
			printf("1	ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, writersInQueue, reading, writing);
			pthread_cond_wait(&canRead, &mutexInit); //czeka na komunikat za bedzie mozna czytac
			readersInQueue--;
		}
		
		reading++;
		printf("3	ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, writersInQueue, reading, writing);
		pthread_cond_broadcast(&canRead);	//daje sygnal czytelnikom, ze mozna wchodzic
		pthread_mutex_unlock(&mutexInit);
		
		sleep(2);
		
		pthread_mutex_lock(&mutexInit);
		reading--;
		printf("4	ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, writersInQueue, reading, writing);
		
		//w przypadku pustej czytelni daje znak dla pisarzy
		if (reading == 0) {
			 pthread_cond_signal(&canWrite); 
		} 
		
		pthread_mutex_unlock(&mutexInit);
		sleep(2);
	}
}


int main(int argc, char *argv[]) {
	int *temp;
	int i;

		
	for(int i = 1; i < argc; ++i){
		if(strcmp(argv[i],"-r") == 0) {
			char *c;
			numberOfReaders = strtol(argv[i+1], &c, 10);
			++i;
		} else if(strcmp(argv[i],"-w") == 0) {
			char *p;
			numberOfWriters = strtol(argv[i+1], &p, 10);
			++i;
		} else if(strcmp(argv[i],"-d") == 0) {
			debugMode = 1;
		}
	}

	writers = malloc (sizeof(pthread_t) * numberOfWriters); //tablica watkow pisarzy
	readers = malloc (sizeof(pthread_t) * numberOfReaders); //tablica watkow czytelnikow
	
	for (i = 0; i < numberOfWriters; ++i) {//tworzy writery 
		temp = (int*)malloc(sizeof(int));
		*temp = i;
		pthread_create(&writers[i], NULL, &writer, temp);
	}
	
	for (i = 0; i < numberOfReaders; ++i) { //tworzy readerow
		temp = (int*)malloc(sizeof(int));
		*temp = i;
		pthread_create(&readers[i], NULL, &reader, temp);
	}
	
	for (i = 0; i < numberOfWriters; ++i) {
		 pthread_join(writers[i], NULL); //czeka na writery
	}
	
	for (i = 0; i < numberOfReaders; ++i) {
		pthread_join(readers[i], NULL); //czeka na readerow
	}
}
