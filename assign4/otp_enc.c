#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "shared.h"
#include <sys/stat.h>
#include <fcntl.h>

char* readFile(char* fileName) {
    //get size of file
    int fileSizeDesc = open(fileName, O_WRONLY);
    int size = lseek(fileSizeDesc, 0, SEEK_END);
    close(fileSizeDesc);
    char* buffer = malloc(size + 1);

    //read file into buffer
    FILE* file = fopen(fileName, "r");
    fgets(buffer, size, file);
    fclose(file);
    //remove new line
    char* newLine = strchr(buffer, '\n');
    if (newLine != NULL) {
        *newLine = '\0';
    }

    //validate chars in string
    int i;
    for (i = 0; buffer[i] != '\0'; i++) {
        if ((buffer[i] < 'A' || buffer[i] > 'Z') && buffer[i] != ' ') {
            fprintf(stderr, "error: invalid character in file %s\n", fileName);
            exit(1);
        }
    }

    return buffer;
}

//put message and key into one string with hashtag at the end
char* concatMessageKey(char* message, char* key) {
    char* both = malloc(strlen(message) + strlen(key) + 4);
    strcpy(both, message);
    both[strlen(message)] = ',';
    strcpy(both + strlen(message) + 1, key);
    both[strlen(message) + strlen(key) + 1] = '#';
    both[strlen(message) + strlen(key) + 2] = '\0'; 
    return both;
}

int connectToPort(int port) {
    //create the socket file descriptor
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        fprintf(stderr, "error: socket couldn't be created\n");
        exit(2);
    }

    //fill in host and port
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    serverHostInfo = gethostbyname("localhost");
    if (serverHostInfo == NULL) {
        fprintf(stderr, "error: could not resolve host name\n");
        exit(2);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    //move host and port to server address
    memcpy((char*)&serverAddress.sin_addr.s_addr, 
           (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

    //attempt to connect to server
    if (connect(socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress))) {
        fprintf(stderr, "error: could not connect to server on port %d\n", port);
        exit(2);
    }

    return socketFd;
}

int main(int argc, char** argv) {
    //check arguments
    if (argc < 4) {
        fprintf(stderr, "usage: %s <messageFile> <keyFile> <port>\n", argv[0]);
        exit(1);
    }
    //assign from arguments
    char* messageFile = argv[1];
    char* keyFile = argv[2];
    int port = atoi(argv[3]);

    //read files specified in args
    char* message = readFile(messageFile);
    char* key = readFile(keyFile);

    //check length of key vs message
    if (strlen(key) < strlen(message)) {
        fprintf(stderr, "error: key is shorter than message\n");
        exit(1);
    }
    
    //connect to server on given port, send validation message
    int serverSocket = connectToPort(port);
    if (!sendText(serverSocket, "encode#")) {
        exit(2);
    }

    //check validation message's return
    char* response = receiveText(serverSocket);
    if (strcmp(response, "200") != 0) {
        fprintf(stderr, "error: not the encryption server\n");
        exit(2);
    }
    
    //send message and key
    char* messageAndKey = concatMessageKey(message, key);
    if (!sendText(serverSocket, messageAndKey)) {
        exit(2);
    }

    //receive and print out message
    char* encodedMessage = receiveText(serverSocket);
    printf("%s\n", encodedMessage);

    close(serverSocket);
    return 0;
}