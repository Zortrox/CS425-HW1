#include <stdio.h>

int main(int argc, const char* argv[]) {

	char input[65] = {0};

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

	printf(input);
	printf("\n");

	getchar();

	return 0;
}