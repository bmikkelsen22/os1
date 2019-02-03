#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"

int main(int argc, char** argv) {
    //check arg count
    if (argc < 2) {
        fprintf(stderr, "usage: %s <numChars>\n", argv[0]);
        exit(1);
    }
    int count = atoi(argv[1]);//get count from args
    char* key = malloc(sizeof(char) * (count + 1));

    //loop through for the requested length and make random digits
    srand(time(NULL));
    int i;
    for (i = 0; i < count; i++) {
        int randomInt = rand() % 27;
        key[i] = intToChar(randomInt);
    }
    key[i + 1] = '\0';
    printf("%s\n", key); //output the randomized key
    return 0;
}