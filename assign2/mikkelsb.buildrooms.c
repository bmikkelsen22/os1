#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>

//
const char * roomNames[] = {
    "kitchen",
    "living",
    "bath",
    "garage",
    "bed",
    "hall",
    "family",
    "pantry",
    "stairs",
    "dining"
};

int roomTable[7][8];
char dirName[30];

void setUpTable() {
    int row, col, i;
    //loop through rows in roomTable
    for (row = 0; row < 7; row++) {
        //keep generating random numbers until one hasnt already been added to the table
        int contains = 0;
        int random = 0;
        do {
            contains = 0;
            random = rand() % 10;
            for (i = 0; i < row; i++) {
                if (roomTable[i][0] == random) {
                    contains = 1;
                }
            }
        } while (contains == 1);
        //add that to the first cell in the current row
        roomTable[row][0] = random;
        //fill the rest of the row with -1
        for (col = 1; col < 8; col++) {
            roomTable[row][col] = -1;
        }
    }
}

int printTable() {
    int row, col;
    //loop through rows in roomTable
    for (row = 0; row < 7; row++) {
        //loop through the columns in the roomTable
        for (col = 0; col < 8; col++) {
            //print out the cell at the current row and column
            printf("%d\t", roomTable[row][col]);
        }
        printf("\n");
    }
}

int CountValidCells(int row) {
    int numCells = 0;
    //count the valid cells in the roomTable
    while (roomTable[row][numCells + 1] != -1) {
        numCells++;
    }
    return numCells;
}

// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int IsGraphFull()  
{
    int row, col;
    //loop through rows in roomTable
    for (row = 0; row < 7; row++) {
        int numCells = CountValidCells(row);
        if (numCells < 3) {
            return 0;
        }
    }
    return 1;
}

// Returns a random row, does NOT validate if connection can be added
int GetRandomRow() {
    return rand() % 7;
}

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
int CanAddConnectionFrom(int row) {
    return CountValidCells(row) < 6;
}

// Returns true if a connection from Room in row1 to Room in row2 already exists, false otherwise
int ConnectionAlreadyExists(int row1, int row2) {
    int col = 1;
    while (roomTable[row2][col] != -1) {
        if (roomTable[row2][col] == roomTable[row1][0]) {
            return 1;
        }
        col++;
    }
    return 0;
}

// Connects Rooms at row1 and row2 together, does not check if this connection is valid
void ConnectRoom(int row1, int row2) {
    int nextColumn = CountValidCells(row1) + 1;
    roomTable[row1][nextColumn] = roomTable[row2][0];
}

// Returns true if Rooms at row1 and row2 are the same Room, false otherwise
int IsSameRoom(int row1, int row2) 
{
    return row1 == row2;
}

// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection()  
{
    int row1, row2;

    while(1)
    {
        row1 = GetRandomRow();
        if (CanAddConnectionFrom(row1) == 1)
            break;
    }

    do
    {
        row2 = GetRandomRow();
    }
    while(CanAddConnectionFrom(row2) == 0 || IsSameRoom(row1, row2) == 1 || ConnectionAlreadyExists(row1, row2) == 1);

    ConnectRoom(row1, row2);  
    ConnectRoom(row2, row1);
}

//write the file from the given row of the roomsTable to the
void WriteRoomFile(int row, char* roomType) {
    char fileName[50];
    sprintf(fileName, "%s/%s", dirName, roomNames[roomTable[row][0]]);
    FILE* fptr = fopen(fileName, "w");
    fprintf(fptr, "ROOM NAME: %s\n", roomNames[roomTable[row][0]]);
    
    //loop through the connections table and add connections to the file
    int i = 1;
    while (roomTable[row][i] != -1) {
        fprintf(fptr, "CONNECTION %d: %s\n", i, roomNames[roomTable[row][i]]);
        i++;
    }
    fprintf(fptr, "ROOM TYPE: %s", roomType);
    fclose(fptr);
}

//write a file for each row with the given room type
void WriteFiles() {
    mkdir(dirName, 0777);
    WriteRoomFile(0, "START_ROOM");
    WriteRoomFile(1, "MID_ROOM");
    WriteRoomFile(2, "MID_ROOM");
    WriteRoomFile(3, "MID_ROOM");
    WriteRoomFile(4, "MID_ROOM");
    WriteRoomFile(5, "MID_ROOM");
    WriteRoomFile(6, "END_ROOM");
}

int main() {
    srand(time(NULL));
    sprintf(dirName, "mikkelsb.rooms.%d", getpid());
    setUpTable();

    // Create all connections in graph
    while (IsGraphFull() == 0)
    {
        AddRandomConnection();
    }

    // Write files
    WriteFiles();
    return 0;
}