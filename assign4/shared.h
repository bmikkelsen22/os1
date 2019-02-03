#pragma once

int charToInt(char c);
char intToChar(int i);
void encrypt(char* message, char* key);
void decrypt(char* message, char* key);
char* receiveText(int socketFd);
int sendText(int socketFd, char* message);
void replaceHashtagWithNull(char* string);
char* appendHashtag(char* string);
