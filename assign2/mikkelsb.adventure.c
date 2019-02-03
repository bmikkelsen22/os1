#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

//constants for use in room struct
const START_ROOM = 0, MID_ROOM = 1, END_ROOM = 2;

//room data structure
struct Room {
    char name[20];
    char* connections[6];
    int connectionsLen;
    int type;
};

//initialize the variables we'll use
int historyLen = 0;
char** history = NULL;

struct Room rooms[7];

char dirName[256];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threadId;

//add string roomName to history array
void addToHistory(char* roomName) {
    char* newStr = malloc(sizeof(char) * 30);
    strcpy(newStr, roomName);
    historyLen++;
    //init array if necessary, otherwise resize it
    if (history == NULL) {
        history = malloc(sizeof(char*) * historyLen);
    } else {
        history = realloc(history, sizeof(char*) * historyLen);
    }
    history[historyLen - 1] = newStr;
}

//free memory from hist array
void freeHistory() {
    int i;
    for (i = 0; i < historyLen; i++) {
        free(history[i]);
    }
    free(history);
}

//read room data from file at roomName into specified row of rooms array
void readRoomData(char* roomName, int row) {
    char fileName[50];
    sprintf(fileName, "%s/%s", dirName, roomName);
    FILE* fptr = fopen(fileName, "r");

    //init connections strings
    int numConn = 0, i, throwaway;
    for (i = 0; i < 6; i++) {
        rooms[row].connections[i] = malloc(sizeof(char) * 20);
    }

    //read connections
    fscanf(fptr, "ROOM NAME: %s\n", rooms[row].name);
    while (fscanf(fptr, "CONNECTION %d: %s\n", &throwaway, rooms[row].connections[numConn]) == 2) {
        numConn++;
    }
    rooms[row].connectionsLen = numConn;
    
    //read type
    char roomType[15];
    fscanf(fptr, "ROOM TYPE: %s", roomType);
    if (strcmp(roomType, "START_ROOM") == 0) {
        rooms[row].type = START_ROOM;
    } else if (strcmp(roomType, "MID_ROOM") == 0) {
        rooms[row].type = MID_ROOM;
    } else if (strcmp(roomType, "END_ROOM") == 0) {
        rooms[row].type = END_ROOM;
    }
    fclose(fptr);
}

// returns row of first room of given type
int findRoom(int roomType) {
    int i;
    for (i = 0; i < 7; i++) {
        if (rooms[i].type == roomType) {
            return i;
        }
    }
    return -1;
}

//updates the path to the directory with the room files
void updateDirName() {
    int newestDirTime = -1; // Modified timestamp of newest subdir examined
    char targetDirPrefix[32] = "mikkelsb.rooms."; // Prefix we're looking for
    memset(dirName, '\0', sizeof(dirName));
  
    DIR* dirToCheck; // Holds the directory we're starting in
    struct dirent *fileInDir; // Holds the current subdir of the starting dir
    struct stat dirAttributes; // Holds information we've gained about subdir
  
    dirToCheck = opendir("."); // Open up the directory this program was run in
  
    if (dirToCheck > 0) // Make sure the current directory could be opened
    {
        while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
        {
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
            {
                stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry
        
                if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
                {
                    newestDirTime = (int)dirAttributes.st_mtime;
                    memset(dirName, '\0', sizeof(dirName));
                    strcpy(dirName, fileInDir->d_name);
                }
            }
        }
    }
  
    closedir(dirToCheck); // Close the directory we opened
}

//loop through room files in directory and call fn to read them in
readAllRooms() {
    DIR* dirToCheck = opendir(dirName);
    struct dirent *fileInDir;

    int row = 0;
    while (fileInDir = readdir(dirToCheck)) {
        if (fileInDir->d_name[0] != '.') {
            readRoomData(fileInDir->d_name, row);
            row++;
        }
    }
}

//returns row number that matches the given name
int getRowFromName(char* name) {
    int i;
    for (i = 0; i < 7; i++) {
        if (strcmp(name, rooms[i].name) == 0){
            return i;
        }   
    }
}

//returns 1 if the name is a connection on the given row
int isNameInRow(char* input, int row) {
    int i;
    //loop thru 
    for (i = 0; i < rooms[row].connectionsLen; i++) {
        if (strcmp(input, rooms[row].connections[i]) == 0) {
            return 1;
        }
    }
    //if the input is time, dont ask for input again
    if (strcmp(input, "time") == 0) {
        return 1;
    }
    return 0;
}

void printPrompt(int row) {
    printf("\nCURRENT LOCATION: %s\nPOSSIBLE CONNECTIONS: ", rooms[row].name);
    int i;
    for(i = 0; i < rooms[row].connectionsLen; i++) {
        printf(rooms[row].connections[i]);
        if (i == rooms[row].connectionsLen - 1) {
            printf(".\n");
        } else {
            printf(", ");
        }
    }
    printf("WHERE TO? >");
}

//write time to currentTime.txt w format specified in instructions
void* writeTimeFile(void *arg) {
    pthread_mutex_lock(&lock);

    //initializations
    time_t rawtime;
    struct tm *info;
    char buffer[80];
 
    time(&rawtime);

    info = localtime(&rawtime);
    //use this format: 1:03pm, Tuesday, September 13, 2016
    strftime(buffer, 80, "%I:%M%p, %A, %B %d, %Y", info);

    //save this time to file
    FILE* fptr = fopen("currentTime.txt", "w");
    fprintf(fptr, buffer);
    fclose(fptr);

    pthread_mutex_unlock(&lock);
    return NULL;
}

//read from the time file and output to console
void readTimeToConsole() {
    char timeString[80];
    FILE* fptr = fopen("currentTime.txt", "r");
    fgets(timeString, sizeof(timeString), fptr);
    printf("\n%s\n", timeString);
    fclose(fptr);
}

void doTimeStuff() {
    
    pthread_mutex_lock(&lock); //main thread starts w mutex
    pthread_create(&threadId, NULL, writeTimeFile, NULL); //make second thread
    
    pthread_mutex_unlock(&lock); //give second thread control of mutex
    pthread_join(threadId, NULL); //wait for second thread to hand over control
    
    readTimeToConsole();
    
}

int main() {
    //get game ready to play, add start room to hist
    updateDirName();
    readAllRooms();
    int curRow = findRoom(START_ROOM);
    int finalRow = findRoom(END_ROOM);
    addToHistory(rooms[curRow].name);

    //loop until the user finishes the game
    char userInput[50];
    do {
        do { //loop until the user gives a legit input
            printPrompt(curRow);
            scanf("%s", userInput);
            //error text if user doesnt input something legit
            if (isNameInRow(userInput, curRow) == 0) {
                printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
            }
        } while (isNameInRow(userInput, curRow) == 0);
        //do time stuff if the user enters "time", 
        //otherwise add guess to hist and move to that room
        if (strcmp("time", userInput) == 0) {
            doTimeStuff();
        } else {
            curRow = getRowFromName(userInput);
            addToHistory(userInput);
        }
    } while (curRow != finalRow);

    //print out the history array
    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", historyLen - 1);
    int i;
    for (i = 0; i < historyLen; i++) {
        printf("%s\n", history[i]);
    }

    freeHistory();
    return 0;
}