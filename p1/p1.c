#include <stdio.h>

int main(int argc, char *argv[]) {
	printf("Hello world!\n");
	printf("There are %u arguments", argc - 1);
	if (argc > 1) {
		printf(", the first of which is %s", argv[1]);
	}
	printf(".\n");
	return 0;
}