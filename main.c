#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_ARGS 20

typedef struct {
	char** vector;
	size_t size;
	size_t maxSize;
} Vector;

void initVector(volatile Vector *vec) {
	vec->vector = (char**) malloc(10 * sizeof(char*));
	vec->size = 0;
	vec->maxSize = 10;
}

void insertVector(volatile Vector *vec, char* str) {
	if (vec->size == vec->maxSize) {
		vec->maxSize += 10;
		vec->vector = (char**)realloc(vec->vector, vec->maxSize * sizeof(char*));
	}
	vec->vector[vec->size] = (char*) malloc(sizeof(str));
	strcpy(vec->vector[vec->size], str);
	vec->size++;
}

void freeVector(volatile Vector *vec) {
	for (int i = 0; i < vec->size; i++) {
		free(vec->vector[i]);
		vec->vector[i] = NULL;
	}
	free(vec->vector);
	vec->vector = NULL;
	vec->size = vec->maxSize = 0;
}

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

bool isHistoryComm(char* cArgs[MAX_ARGS]) {
	return (cArgs[0] != NULL && strcmp(cArgs[0], "r") == 0);
}

void parseInput(char* input, char* inputSplit, char* cArgs[MAX_ARGS], bool* bRunConcurr, bool original) {
	bool goodInput = true;

	if (!original) goodInput = false;



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

	if (isHistoryComm(cArgs) || cArgs[0] == NULL) goodInput = false;

	if (goodInput) {
		insertVector(&vecComms, input);
	}
}

int main(int argc, const char* argv[]) {

	signal(SIGINT, intHandler);

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
		parseInput(input, inputSplit, cArgs, &bRunConcurr, true);

		//run a previous command
		if (isHistoryComm(cArgs)) {
			long num = strtol(cArgs[1], NULL, 10);
			if (num <= vecComms.size && num > 0) {
				char* comm = vecComms.vector[num - 1];

				char commSplit[sizeof(input) * sizeof(char)];
				parseInput(comm, commSplit, cArgs, &bRunConcurr, false);
			}
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
					wait(0);
					wait(0);
				}
				//error
				else if (pid < 0) {
					printf("Fork error");
				}
			}
		}
	}

	return 0;
}