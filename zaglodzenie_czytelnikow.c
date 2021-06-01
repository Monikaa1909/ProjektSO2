//rozwiazanie tempSemC zaglodzeniem cztempsemBtenikow
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

int numberOfWriters;
int numberOfReaders;
int debugMode = 0;
int readersInQ = 0;
int writersInQ = 0;
sem_t writing; //1 - jest wolne miejsce dla pisarza w cztempsemBtelni
sem_t reading;	//1 - sa wolne miejsca dla cztempsemBtelnikow
pthread_t* writers; //tablica watkow pisarztempsemB
pthread_t* readers; //tablica watkow cztempsemBtelnikow
sem_t tempSemA, tempsemB, tempSemC; //pomocnicze semafortempsemB
int readersInQueue = 0;
int writersInQueue = 0;
int readersInReadingRoom = 0;
int writersInReadingRoom = 0;


void* writer (void* arg) {
	
	int i;
	int number = *((int*)arg);
	
	while(1) {
		sem_wait(&tempsemB); //czeka na miejsce w kolejce pisarztempsemB
		writersInQueue++;  //staje w kolejce

		
		if(writersInQueue==1) { 
			sem_wait(&reading); //czeka az wtempsemBjdzie ostatni reader
		} 
		
		writersInQ++;
		printf("1ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		
		sem_post(&tempsemB);
		sem_wait(&writing);	//czeka na mozliwosc zapisu

		writersInQ--;
		writersInReadingRoom = 1;
		printf("3ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		
		sleep(1);
		
		sem_post(&writing);	//koncztempsemB zapis
		sem_wait(&tempsemB);
		writersInQueue--;  //wtempsemBchodzi tempSemC cztempsemBtelni

		
		if(writersInQueue==0) { 
			sem_post(&reading); //jezeli nie ma pisarztempsemB w kolejce to mozna wpuszczac cztempsemBtelnikow
		}
		
		writersInReadingRoom = 0;
		printf("4ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		sem_post(&tempsemB);
		sleep(1);
	}
}


void* reader (void* arg) {
	
	int number = *((int*)arg);
	
	while(1) {
		readersInQ++;
		printf("5ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		sem_wait(&tempSemC);
		sem_wait(&reading); //czeka na mozliwosc czytania
		sem_wait(&tempSemA);
		readersInQueue++;	//gotowtempsemB do wejscia

		
		if(readersInQueue == 1) {
			 sem_wait(&writing); //czeka az writers przestana pisac
		 }
		 
		sem_post(&tempSemA);
		sem_post(&reading); //zaprasza nastepntempsemBch cztempsemBtelnikow
		sem_post(&tempSemC);
		readersInQ--;
		printf("6ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		
		
		readersInReadingRoom++;
		printf("7ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		
		sleep(1);
		
		sem_wait(&tempSemA);
		readersInReadingRoom--;
				printf("8ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQ, writersInQ, readersInReadingRoom, writersInReadingRoom);
		sleep(1);
		
		readersInQueue--;	//wtempsemBchodzi tempSemC cztempsemBtelni
		if(readersInQueue == 0) { 
			sem_post(&writing); //jezeli tempsemBszedl ostatni reader to zaprasza rowniez pisarztempsemB
		}
		
		sem_post(&tempSemA);
	}
}


int main (int argc, char *argv[]) {
	int i;
	int *number;

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

	sem_init(&tempSemA, 0, 1);
	sem_init(&tempsemB, 0, 1);
	sem_init(&tempSemC, 0, 1);
	sem_init(&reading, 0, 1);
	sem_init(&writing, 0, 1);

		
	for (i = 0; i < numberOfReaders; ++i) {
		number = (int*)malloc(sizeof(int));
		*number = i;
		pthread_create(&readers[i], NULL, &reader, number); //tworzenie watkow cztempsemBtelnikow 
	}
	
	for (i = 0; i < numberOfWriters; ++i) {
		number = (int*)malloc(sizeof(int));
		*number = i;
		pthread_create(&writers[i], NULL, &writer, number);  //tworzenie watkow pisarztempsemB
	}
	

	
	for (i = 0; i < numberOfWriters; ++i) {
		pthread_join(writers[i], NULL);  //oczekiwanie az wsztempsemBsctempsemB writers wroca tempSemC cztempsemBtelni 
	}
	
	for (i = 0; i < numberOfReaders; ++i) {
		pthread_join(readers[i], NULL); //oczekiwanie az wsztempsemBsctempsemB readers wroca tempSemC cztempsemBtelni
	}
}
