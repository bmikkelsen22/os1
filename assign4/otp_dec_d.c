#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "shared.h"
#include <stdbool.h>

//handle all communication
void handleConnection(int clientSocket) {
    //fork new process to handle connection in
    int newPid = fork();
    switch (newPid) {
        case -1: //error forking
            fprintf(stderr, "error: could not create child process\n");
            return;
        case 0: //this is the child process, handle the conn
            ; //c is stupid
            char* req = receiveText(clientSocket);
            if (strcmp(req, "decode") != 0) {
                sendText(clientSocket, "400#"); //bad request
                exit(0);
            } else {
                sendText(clientSocket, "200#"); //good request
            }
            free(req);

            //split message and key
            char* message = receiveText(clientSocket);
            char* comma = strchr(message, ',');
            char* key = comma + 1;
            *comma = '\0'; 
            
            decrypt(message, key); //decrypts it in place
            
            sendText(clientSocket, appendHashtag(message));
            close(clientSocket);
            exit(0);
        default: 
            return;
    }
}

void listenServer(int port) {
    //open up the socket's file descriptor
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);

    //set the port to listen on (passed in) and bind it to the file descriptor
    struct sockaddr_in serverAddress, clientAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "error: couldn't bind port\n");
        exit(1);
    }

    //start listening
    if (listen(listenSocket, 5) < 0) {
        fprintf(stderr, "error: could not start listening\n");
        exit(1);
    }

    //loop so so that we can handle multiple connections
    while(1) {
        int addressSize = sizeof(clientAddress);
        int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &addressSize);
        if (clientSocket < 0) {//recover from error and wait for another request
            fprintf(stderr, "error: could not start listening\n");
            continue;
        }
        //this forks and then handles communication with client
        handleConnection(clientSocket);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { //check command line args
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    listenServer(port);
    return 0;
}