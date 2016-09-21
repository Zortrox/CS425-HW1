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
	signal(sig, intHandler);
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

		char inputSplit[sizeof(input) * sizeof(char)];
		strcpy(inputSplit, input);

		int argIndex = 0;
		char* argPtr;
		argPtr = strtok(inputSplit, " ");
		while (argPtr != NULL && index < MAX_ARGS - 1)
		{
			//if not the Ctrl + C
			if (strcmp(argPtr, "^C") != 0) {
				cArgs[argIndex] = argPtr;
				argPtr = strtok(NULL, " ");
				argIndex++;
			}
		}
		cArgs[argIndex] = NULL;	//for end of arguments

		//determine if command will run concurrently
		//then remove from arguments
		bool bRunConcurr = false;
		for (int i = 0; i < MAX_ARGS; i++) {
			if (cArgs[i] == NULL) {
				if (strcmp(cArgs[i - 1], "&") == 0) {
					bRunConcurr = true;
					cArgs[i - 1] = NULL;
				}
				break;
			}
		}

		insertVector(&vecComms, input);


		int pid = fork();
		//child
		if (pid == 0) {
			execvp(cArgs[0], cArgs);
			_exit(0);
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