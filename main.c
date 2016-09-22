#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

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

	//if the input captured the Crtl+C signal, remove and shift string
	if (input[0] == '\xFF') {
		int size = sizeof(input) / sizeof(char);
		for (int i = 0; i < size - 1; i++) {
			if (input[i] != '\0') {
				input[i] = input[i + 1];
			}
			else {
				break;
			}
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

	if (isHistoryComm(cArgs)) goodInput = false;

	//determine if command will run concurrently
	//then remove from arguments
	for (int i = 0; i < MAX_ARGS; i++) {
		if (cArgs[i] == NULL) {
			if (i > 0 && strcmp(cArgs[i-1], "&") == 0) {
				*bRunConcurr = true;
				cArgs[i-1] = NULL;
			}
			else if (i == 0) {
				goodInput = false;
			}
			break;
		}
	}

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
		printf("sh> ");
		fflush(stdin);
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

		int pid = fork();
		//child
		if (pid == 0) {

			execvp(cArgs[0], cArgs);
		}
		//parent
		else if (pid > 0 && !bRunConcurr) {
			wait(0);
		}
		//error
		else if (pid < 0) {
			printf("Fork error");
		}
	}

	return 0;
}