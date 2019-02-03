#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

struct Command {
	char command[100];
	char* args[513];
	char inFile[100];
	char outFile[100];
	int background;
};

int prevExitStatus = 0;
int allowBackground = 1;
int backgroundPids[1000] = {0};

void readCommand(struct Command* cmd, char* input) {
	memset(cmd, 0, sizeof(struct Command));
	if (input == NULL) return;
	if (input[0] == '#') return;

	int numArgs = 0;

	//read in args
	char * word = strtok(input, " ");
	while (word != NULL) {
		cmd->args[numArgs] = (char*)malloc(sizeof(char) * (strlen(word) + 10));
		
		//replace $$ with the process id
		char* pidStart = strstr(word, "$$");
		if (pidStart != NULL) {
			char pidString[10];
			sprintf(pidString, "%d", getpid());
			int pidLen = strlen(pidString);
			size_t charsBefore = pidStart - word;
			strncpy(cmd->args[numArgs], word, charsBefore); //copy before $$
			strcpy(cmd->args[numArgs] + charsBefore, pidString); //copy $$
			strcpy(cmd->args[numArgs] + charsBefore + pidLen, word + charsBefore + 2); // copy after $$
		}
		else {
			strcpy(cmd->args[numArgs], word);
		}

		//replace newline with 0
		char* newLine = strchr(cmd->args[numArgs], '\n');
		if (newLine != NULL) {
			*newLine = '\0';
		}

		word = strtok(NULL, " ");
		numArgs++;
	}

	//get command from args
	if (numArgs > 0) {
		strcpy(cmd->command, cmd->args[0]);
	}

	//get infile and outfile
	int i;
	int inLoc = 0, outLoc = 0, removeAt = -1;
	for (i = 0; i < numArgs - 1; i++) {
		if (strcmp("<", cmd->args[i]) == 0) {
			strcpy(cmd->inFile, cmd->args[i + 1]);
			inLoc = i;
		}
		if (strcmp(">", cmd->args[i]) == 0) {
			strcpy(cmd->outFile, cmd->args[i + 1]);
			outLoc = i;
		}
	}

	//remove < and > from args if needed
	if (inLoc) {
		removeAt = inLoc;
	}
	if (outLoc) {
		if (removeAt == -1 || outLoc < removeAt) {
			removeAt = outLoc;
		}
	}

	//check for & at end of args
	if (numArgs >= 1 && strcmp(cmd->args[numArgs - 1], "&") == 0) {
		if (allowBackground) {
			cmd->background = 1;
		}
		
		if (removeAt == -1) {
			removeAt = numArgs - 1;
		}
	}

	//remove extra args for <,>, and &
	if (removeAt > 0 ) {
		for (i = removeAt; cmd->args[i] != NULL; i++) {
			free(cmd->args[i]);
			cmd->args[i] = NULL;
		}
	}
}

//this is only for testing, prints out all the parts of the command struct
void printCommand(struct Command* cmd) {
	printf("command: %s, background %d, infile: %s, outfile: %s\n", cmd->command, cmd->background, cmd->inFile, cmd->outFile);
	int i = 0;
	printf("args:\n");
	while (cmd->args[i] != NULL) {
		printf("%s\n", cmd->args[i]);
		i++;
	}
	printf("\n\n");
	fflush(stdout);
}

//change the directory according to the args of the cmd struct
void changeDirectory(struct Command* cmd) {
	char * dirName;
	//check if a directory was specified. if so, use it, otherwise use home directory
    if (cmd->args[1] != NULL) {
        dirName = cmd->args[1];
    } else {
        dirName = getenv("HOME");
    }
	int result = chdir(dirName);
    if (result != 0) { // if there was an error changing the directory
        printf("error: invalid directory name %s\n", dirName);
        fflush(stdout);
    }
}

//kill all processes with ids stored in backgroundPids array.
void killAllBackgroundProcesses() {
	int i;
	for (i = 0; backgroundPids[i] != 0; i++) {
		kill(backgroundPids[i], SIGKILL);
	}
}

//call kill all processes function, then exit
void cleanAndExit() {
	killAllBackgroundProcesses();
	exit(0);
}

//print exit/term status
void printStatus(int status) {
	if (WIFEXITED(status)) {
		printf("exited with status %d\n", WEXITSTATUS(status));
	} else {
		printf("terminated with signal %d\n", WTERMSIG(status));
	}
	fflush(stdout);
}

//add given pid to backgroundPids array
void addBackgroundPid(int pid) {
	int i = 0;
	while (backgroundPids[i] != 0) {
		i++;
	}
	backgroundPids[i] = pid;
}

//remove given pid from backgroundPids array, and then squash array so it is not sparse
void removeBackgroundPid(int pid) {
	int i = 0;
	//find pid
	while (backgroundPids[i] != pid && backgroundPids[i] != 0) {
		i++;
	}
	while (backgroundPids[i] != 0) {
		backgroundPids[i] = backgroundPids[i + 1];
		i++;
	}
}

//check processes that have finished, remove them from backgroundPids array
void checkBackgroundPids() {
	int pid = 0, statusSig, sigNum;
	while (1) {
		pid = waitpid(-1, &statusSig, WNOHANG);
		if (pid > 0) {
			removeBackgroundPid(pid);
			printf("process %d ", pid);
			printStatus(statusSig);
		} else {
			break;
		}
	}
}

//open infile and redirect, if there is an infile
void setupInFile(struct Command* cmd) {
	int fileDesc;
	if (strlen(cmd->inFile) != 0) { //if infile specified
		fileDesc = open(cmd->inFile, O_RDONLY, S_IRUSR | S_IWUSR);
		if (fileDesc == -1) {
			printf("error: unknown file %s\n", cmd->inFile);
			exit(1);
		}
	} else if (cmd->background) { //otherwise if we run in the bg
		fflush(stdout);
		fileDesc = open("/dev/null", O_RDONLY);
	} else { //otherwise don't redirect
		return;
	}
	dup2(fileDesc, 0);
}

//if there is an outfile, open outfile and redirect. create if nonexistant
void setupOutFile(struct Command* cmd) {
	int fileDesc;
	if (strlen(cmd->outFile) != 0) { // open infile if specified
		fileDesc = open(cmd->outFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	} else if (cmd->background) { //otherwise if were running in bg, dump to dev null
		fileDesc = open("/dev/null", O_WRONLY);
	} else {//othersiwe don't redirect
		return;
	}
	dup2(fileDesc, 1);
}

//run command
void runCommand(struct Command* cmd) {
	pid_t spawnPid = -5;
	spawnPid = fork();
	switch (spawnPid) {
		case -1: //error
			printf("error: could not fork process\n");
			fflush(stdout); 
			break;
		case 0: //child
			setupInFile(cmd);
			setupOutFile(cmd);

			//if not in background, re-enable default sig int handler
			if (!cmd->background) {
				struct sigaction sigIntAction = {0};
				sigIntAction.sa_handler = SIG_DFL;
				sigaction(SIGINT, &sigIntAction, NULL);
			}

			//exec and print error if there is one
			int result = execvp(cmd->command, cmd->args);
			printf("error: unknown command %s\n", cmd->command);
			fflush(stdout);
			exit(1);
			break;
		default: //parent
			if (cmd->background) { //run in background
				addBackgroundPid(spawnPid);
				printf("background process started with id %d\n", spawnPid);
				fflush(stdout);
			} else { //run in foreground
				waitpid(spawnPid, &prevExitStatus, 0); //store exit details
			}
	}
}

//call the right function based on the command the user typed
void callCommandFunction(struct Command* cmd) {
	if (strcmp(cmd->command, "cd") == 0) {
		changeDirectory(cmd);
	}
	else if (strcmp(cmd->command, "exit") == 0) {
		cleanAndExit();
	}
	else if (strcmp(cmd->command, "status") == 0) {
		printStatus(prevExitStatus);
	}
	else if (strlen(cmd->command) != 0) {
		runCommand(cmd);
	}
}

//handle a sig stp
void catchSigStp(int sigNo) {
	allowBackground = !allowBackground; // toggle whether background processes are enabled
	if (allowBackground) {
		write(1, "\ncommands can now be run in background\n", 39);
	} else {
		write(1, "\nentering foreground-only mode (& is now ignored)\n", 50);
	}
}

//catch signal for interrupt, output that background process terminated!
void catchSigInt(int sigNo) {
	write(1, "terminated with signal 2\n", 25);
}

//set up sigal handlers
void setUpSignals() {
	struct sigaction sigIntAction = {0}, sigStpAction = {0};
	//for sig int
	sigIntAction.sa_handler = SIG_IGN;
	sigfillset(&sigIntAction.sa_mask);
	sigaction(SIGINT, &sigIntAction, NULL);
	//for sig stop
	sigStpAction.sa_handler = catchSigStp;
	sigfillset(&sigStpAction.sa_mask);
	sigaction(SIGTSTP, &sigStpAction, NULL);
}

int main() {
	setUpSignals();
	while (1) { //continue until user enters exit
		printf(": ");
		fflush(stdout);

		char buffer[2049];
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);

		struct Command cmd;
		readCommand(&cmd, buffer);

		callCommandFunction(&cmd);
		checkBackgroundPids();
	}
}
