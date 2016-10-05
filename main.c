/*
Matthew Clark
---
Console Shell Program
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_ARGS 20

//variable-sized array
typedef struct {
	char** vector;
	size_t size;
	size_t maxSize;
} Vector;

//creation
void initVector(volatile Vector *vec) {
	vec->vector = (char**)malloc(10 * sizeof(char*));
	vec->size = 0;
	vec->maxSize = 10;
}

//add to vector
void insertVector(volatile Vector *vec, char* str) {
	if (vec->size == vec->maxSize) {
		vec->maxSize += 10;
		vec->vector = (char**)realloc(vec->vector, vec->maxSize * sizeof(char*));
	}
	vec->vector[vec->size] = (char*)malloc(sizeof(str));
	strcpy(vec->vector[vec->size], str);
	vec->size++;
}

//delete vector
void freeVector(volatile Vector *vec) {
	for (int i = 0; i < vec->size; i++) {
		free(vec->vector[i]);
		vec->vector[i] = NULL;
	}
	free(vec->vector);
	vec->vector = NULL;
	vec->size = vec->maxSize = 0;
}

//command vector & interrupt handler to output commands
static volatile Vector vecComms;
void intHandler(int sig) {
	signal(sig, SIG_IGN);
	printf("\n");
	int last = (((signed int)vecComms.size - 10) > 0) ? vecComms.size - 10 : 0;
	for (int i = vecComms.size - 1; i >= last; i--) {
		char* command = vecComms.vector[i];
		printf("%i: %s\n", i + 1, command);
	}
	printf("\nsh> ");
	signal(sig, intHandler);
}

//determine if the command entered is used to run a previous command
bool isHistoryComm(char* cArgs[MAX_ARGS]) {
	return (cArgs[0] != NULL && strcmp(cArgs[0], "r") == 0);
}

//break the input into individual indices and determine if the command needs to run concurrently
void parseInput(char* input, char* inputSplit, char* cArgs[MAX_ARGS], bool* bRunConcurr) {
	bool goodInput = true;

	//remove Ctrl+C and whitepace at beginning
	while (input[0] == '\xFF' || isspace(input[0])) {
		for (int i = 0; input[i]; i++) {
			input[i] = input[i + 1];
		}
	}

	//get end character
	int strEnd = 0;
	for (int i = 0; input[i]; i++) {
		strEnd = i;
	}
	//remove whitespace from end
	for (int i = strEnd; i >= 0; i--) {
		if (isspace(input[i])) {
			input[i] = '\0';
		}
		//check to run process concurrently
		else if (input[i] == '&') {
			*bRunConcurr = true;
			input[i] = '\0';
		}
		else {
			break;
		}
	}

	//keep input intact and split the input string based on spaces
	strcpy(inputSplit, input);
	int argIndex = 0;
	char* argPtr;
	argPtr = strtok(inputSplit, " ");
	while (argPtr != NULL && argIndex < MAX_ARGS - 1)
	{
		cArgs[argIndex] = argPtr;
		argPtr = strtok(NULL, " ");
		argIndex++;
	}
	cArgs[argIndex] = NULL;	//for end of arguments

							//determine if the input is to be inserted into the command array
	if (isHistoryComm(cArgs) || cArgs[0] == NULL) goodInput = false;

	//insert into command array if the input is good
	if (goodInput) {
		insertVector(&vecComms, input);
	}
}

int main(int argc, const char* argv[]) {

	//catch interrupts
	signal(SIGINT, intHandler);

	//start user input loop
	initVector(&vecComms);
	bool bKeepGoing = true;
	while (bKeepGoing) {
		char* cArgs[MAX_ARGS];
		char input[65] = { 0 };

		char c;
		int index = 0;
		fflush(stdout);
		printf("sh> ");
		while ((c = getchar()) != '\n') {
			int length = sizeof(input) / sizeof(input[0]) - 1;	//save 1 for ending character
			if (index < length) {
				input[index] = c;
			}
			index++;
		}
		//flush input buffer
		while (c != '\n' && c != EOF) {
			c = getchar();
		}

		bool bRunConcurr = false;
		char inputSplit[sizeof(input) * sizeof(char)];
		parseInput(input, inputSplit, cArgs, &bRunConcurr);

		//run a previous command by number of command
		if (isHistoryComm(cArgs)) {
			long num = 0;
			if (cArgs[1] == NULL) {
				num = vecComms.size;
			}
			else if (cArgs[1][0] >= 48 && cArgs[1][0] <= 57) {	//if the function is numeric
				num = strtol(cArgs[1], NULL, 10);
			}
			else {	//the function goes by letter
					//go through command vector in reverse and get the first
					//element that starts with the inputted command
				for (int i = vecComms.size; i > 0; i--) {
					if (vecComms.vector[i - 1][0] == cArgs[1][0]) {
						num = i;
						break;
					}
				}
			}

			//make sure num is in array bounds
			if (num <= vecComms.size && num > 0) {
				char* comm = vecComms.vector[num - 1];

				char commSplit[sizeof(input) * sizeof(char)];
				parseInput(comm, commSplit, cArgs, &bRunConcurr);
			}
			//otherwise print that command wasn't found
			else {
				printf("Command not found\n");
			}
		}

		//if command not empty
		if (cArgs[0] != NULL) {
			//convert first argument to lower case for testing
			for (int i = 0; cArgs[0][i]; i++) {
				cArgs[0][i] = tolower(cArgs[0][i]);
			}

			//if command is exit
			if (strcmp(cArgs[0], "exit") == 0) {
				bKeepGoing = false;
			}
			else {
				int pid = fork();
				//child
				if (pid == 0) {
					execvp(cArgs[0], cArgs);

					exit(0);
				}
				//parent
				else if (pid > 0 && !bRunConcurr) {
					int status = 0;
					waitpid(pid, &status, 0);
				}
				//error
				else if (pid < 0) {
					printf("Fork error");
				}
			}
		}
	}

	//wait for all child threads before exiting
	waitpid(-1, NULL, 0);

	return 0;
}