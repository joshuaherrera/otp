#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

//This program generates a random string of user defined length. The string is
//composed of upper case letters and the space character only.
//Assumes ASCII encoding.
int main(int argc, char const *argv[])
{
	char *endptr, *numInput;
	long keySize;
	srand(time(NULL));
	//check that the user has enter only 2 arguments. in C the first argument
	//is always the name of the program.
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s [size of key]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	numInput = argv[1];
	errno = 0;
	keySize = strtol(numInput, &endptr, 10);
	//check for errors with conversion

	if ((errno == ERANGE && (keySize == LONG_MAX || keySize == LONG_MIN))
		|| (errno != 0 && keySize == 0))
	{
		perror("strtol");
		exit(EXIT_FAILURE);
	}

	if (endptr == numInput)
	{
		fprintf(stderr, "No digits found\n");
		exit(EXIT_FAILURE);
	}

	//here we successfully converted the input to an integer
	if (*endptr != '\0')
	{
		fprintf(stderr, "Error: Further characters after number: %s\n", endptr);
		exit(EXIT_FAILURE);
	}

	//make a character of n+1 bytes where n is the user defined size of the str
	char key[keySize + 1];
	char startChar = 'A';
	memset(key, '\0', sizeof(key));
	int i;
	for(i = 0; i < keySize + 1; i++)
	{
		//get a random number from 0 - 27 that is used to generate the random char
		//to store in the str array.
		int randVal = rand() % 27;
		//check if the random number is 26, if so, add a space to the str array.
		if (randVal == 26)
		{
			key[i] = ' ';
		}
		//in the other case, we add the random number to 'A' to get a random char
		//and add to the str arr.
		else
		{
			key[i] = startChar + randVal;
		}
	}

	//lastly append a newline to the array and send to stdout.
	key[keySize] = '\0';
	printf("%s\n", key);
	return 0;
}