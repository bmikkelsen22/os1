#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "shared.h"
#include <stdbool.h>

//takes an uppercase letter or space and condenses the int value into 0-26
int charToInt(char c) {
    if (c == ' ') {
        return 26;
    } else {
        return c - 'A';
    }
}

//takes condensed value and expands it to char value
char intToChar(int i) {
    if (i == 26) {
        return ' ';
    } else {
        return i + 'A';
    }
}

//encrypts message in place, overwriting original message
void encrypt(char* message, char* key) {
    int i;
    for (i = 0; message[i] != '\0' && key[i] != '\0'; i++) {
        int encryptedInt = (charToInt(message[i]) + charToInt(key[i])) % 27;
        message[i] = intToChar(encryptedInt);
    }
}

//decrypts message in place, overwriting original message
void decrypt(char* message, char* key) {
    int i;
    for (i = 0; message[i] != '\0' && key[i] != '\0'; i++) {
        int decryptedInt = (charToInt(message[i]) - charToInt(key[i]));
        if (decryptedInt < 0) {
            decryptedInt += 27;
        }
        message[i] = intToChar(decryptedInt);
    }
}

//find a hashtag in the function and replace it with null character
void replaceHashtagWithNull(char* string) {
    int i;
    for (i = 0; string[i] != '\0'; i++) {
        if (string[i] == '#') {
            string[i] = '\0';
            return;
        }
    }
}

//handle single receive, return text read (malloced in this function)
char* receiveText(int socketFd) {
    //set up variables/buffers
    int charsRead = 0;
    char* textReceived = malloc(1);
    textReceived[0] = '\0';
    char buffer[256];

    do {//repeat until we recieve a hashtag
        memset(buffer, '\0', 256);
        int newCharsRead = recv(socketFd, buffer, 255, 0); //receive up to 255 chars from server
        if (newCharsRead < 0) {
            fprintf(stderr, "error: couldn't bind port\n");
            break;
        }
        //resize char array as needed
        textReceived = realloc(textReceived, charsRead + 256);
        strcpy(textReceived + charsRead, buffer); //copy over new chars from buffer
        charsRead += newCharsRead;
    } while (strchr(textReceived, '#') == NULL);
    
    replaceHashtagWithNull(textReceived);
    return textReceived;
}

//send given text over given socket
int sendText(int socketFd, char* message) {
    int messageLen = strlen(message);
    int sentLen = 0;
    while (sentLen < messageLen) {
        int charsSent = send(socketFd, message + sentLen, messageLen - sentLen, 0);//send as much as possible, continue until finished
        if (charsSent < 0) {//stop sending if we get an error
            fprintf(stderr, "error: issue sending message\n");
            return false;
        }
        sentLen += charsSent;
    }
    return true;
}

//put hashtag at end of string as end of message marker
char* appendHashtag(char* string) {
    int len = strlen(string);
    char* newString = malloc(len + 2);
    strcpy(newString, string);
    newString[len] = '#';
    newString[len + 1] = '\0';
    return newString;
}