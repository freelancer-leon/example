/*
 * yum install libasan libubsan
 * gcc uaf.c -o uaf -fsanitize=address -fno-omit-frame-pointer -g
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[])
{
	char *s = malloc(100);
	free(s);
	strcpy(s, "Hello world!");
	printf("string is: %s\n", s);
	return 0;
}

