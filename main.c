#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 20

int main(int argc, const char* argv[]) {

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

		int argIndex = 0;
		char* argPtr;
		argPtr = strtok(input, " ");
		while (argPtr != NULL && index < MAX_ARGS - 1)
		{
			cArgs[argIndex] = argPtr;
			argPtr = strtok(NULL, " ");
			argIndex++;
		}
		cArgs[argIndex] = NULL;	//for end of arguments

		bool bRunConcurr = false;
		for (int i = 0; i < MAX_ARGS; i++) {
			if (cArgs[i] == NULL) {
				//determine if command will run concurrently
				//then remove from arguments
				if (strcmp(cArgs[i - 1], "&") == 0) {
					bRunConcurr = true;
					cArgs[i - 1] = NULL;
				}
				break;
			}
		}

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