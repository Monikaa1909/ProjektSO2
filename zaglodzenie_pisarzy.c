//rozwiazanie z zaglodzeniem pisarzy
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

int numberOfWriters;
int numberOfReaders;

sem_t writing; //1 - jest wolne miejsce dla pisarza w czytelni
sem_t reading;	//1 - sa wolne miejsca dla czytelnikow
pthread_t* writers;
pthread_t* readers;
int debugMode = 0;
int readersInReadingRoom = 0; //liczba czytelnikow w czytelni
int writersInReadingRoom = 0;
int wirtersInQueue = 0;
int readersInQueue = 0;

void* writer (void* arg)
{
	int i;
	int number = *((int*)arg);
	while(1) {
		++wirtersInQueue;
		printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, wirtersInQueue, readersInQueue, writersInReadingRoom);
		sem_wait(&writing);	//blokuje innym pisarzom mozliwosc pisania
		--wirtersInQueue;
		printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, wirtersInQueue, readersInQueue, writersInReadingRoom);
		sleep(1);
		sem_post(&writing);	//wychodzi i umozliwia pisanie innym pisarzom
		++writersInReadingRoom;
		printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, wirtersInQueue, readersInQueue, writersInReadingRoom);
		--writersInReadingRoom;
		sleep(1);
	}
}

void* reader (void* arg) {
	int i, number = *((int*)arg);
	while(1){
		++readersInQueue;
		printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, wirtersInQueue, readersInQueue, writersInReadingRoom);
		
		--readersInQueue;
		sem_wait(&reading);	//czeka na mozliwosc czytania
		readersInReadingRoom++;	//gotowy do wejscia		
		if(readersInReadingRoom == 1) {
			sem_wait(&writing);	//jezeli jest pierwszy to blokuje mozliwosc pisania
		} 
		++readersInReadingRoom;
		sem_post(&reading); //zaprasza nastepnych czytelnikow
		sleep(1);

		sem_wait(&reading);	//czyta
		++readersInReadingRoom;
		sleep(1);
		readersInReadingRoom--; //wychodzi
		printf("ReaderQ: %d WriterQ: %d [in: R:%d W:%d]\n", readersInQueue, wirtersInQueue, readersInQueue, writersInReadingRoom);
		if(readersInReadingRoom == 0) sem_post(&writing);	//jezeli wychodzi ostatni to umozliwia pisanie
		sem_post(&reading);	//zaprasza tez czytelnikow

	}
}


int main (int argc, char* argv[]) {
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

	sem_init(&reading, 0, 1);
	sem_init(&writing, 0, 1);
	for (i = 0; i < numberOfWriters; i++) {
		number = (int*)malloc(sizeof(int));
		*number = i;
		pthread_create(&writers[i], NULL, &writer, number);  //tworzenie watkow pisarzy
	}
	for (i = 0; i < numberOfReaders; i++) {
		number = (int*)malloc(sizeof(int));
		*number = i;
		pthread_create(&readers[i], NULL, &reader, number); //tworzenie watkow czytelnikow 
	}
	for (i = 0; i < numberOfWriters; i++) {
		pthread_join(writers[i], NULL);  //oczekiwanie az wszyscy writers wroca z czytelni 
	} 
	for (i = 0; i < numberOfReaders; i++){
		pthread_join(readers[i], NULL); //oczekiwanie az wszyscy readers wroca z czytelni
	} 
}
