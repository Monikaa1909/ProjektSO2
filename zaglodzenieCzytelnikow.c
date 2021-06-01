#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

long readers;
long writers;
debug = false;

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
        printf("Ilosc czytelnikow = %ld, ilosc pisarzy = %ld, wartosc debug = %d\n", readers, writers, debug);
    }

}