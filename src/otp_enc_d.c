#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>


#define BUFF_SIZE 		262144

char buffer[BUFF_SIZE];
char textContents[BUFF_SIZE];
char keyContents[BUFF_SIZE];

void error(const char *msg, int exitStat) { perror(msg); exit(exitStat); } // Error function used for reporting issues

void childReaper(int sig)
{
	assert(sig == SIGCHLD);
	//save error number
	int saved = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0)
	{
	    continue;
	}
	errno = saved;
}


void chopN(char *str, size_t n)
{
    assert(n != 0 && str != 0);
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

int main(int argc, char *argv[])
{
	struct sigaction SIGCHLD_action = {0};
	SIGCHLD_action.sa_handler = childReaper;
	sigfillset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &SIGCHLD_action, NULL) == -1)
	{
		fprintf(stderr, "Error: sigaction error\n");
		exit(1);
	}

	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, totalRead, charsWritten, totalWritten;
	socklen_t sizeOfClientInfo;
	//char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnpid = -5;

	char usableChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket", 1);

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding", 1);
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	//this file descriptor should be used by parent proc ONLY! Remember child gets copy, must close


	//begin forking procedure
	for (;;)
	{
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept", 2);
		spawnpid = fork();

		switch(spawnpid)
		{
			case -1:
			{
				error("Error forking", 1);
				break;
			}
			//child
			//receives string, encrypts, and sends back encrypted message
			case 0:
			{
				//close duplicate FD
				close(listenSocketFD);
				// Get the message from the client and display it
				memset(buffer, '\0', BUFF_SIZE);
				totalRead = 0;
				//do an initial read to identify the connection verifiers
				charsRead = recv(establishedConnectionFD, buffer + totalRead, BUFF_SIZE - 1, 0); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket", 1);
				totalRead += charsRead;
				//remember to check for identifier characters
				if (strstr(buffer, "^") != NULL)
				{
					while(strstr(buffer, "@@") == NULL)
					{
						charsRead = recv(establishedConnectionFD, buffer + totalRead, BUFF_SIZE - 1, 0); // Read the client's message from the socket
						if (charsRead < 0) error("ERROR reading from socket", 1);
						totalRead += charsRead;
					}
				}
				//we need to send the client a message letting them know they cannot
				//connect to this server
				else
				{
					//send ~~ to let client know it is not the right server
					totalWritten = 0;
					char closeStr[10];
					strcpy(closeStr, "~~@@");
					charsWritten = send(establishedConnectionFD, closeStr, strlen(closeStr), 0);
					totalWritten += charsWritten;
					while(totalWritten < strlen(closeStr))
					{
					    //keep sending
					    charsWritten = send(establishedConnectionFD, closeStr + totalWritten, strlen(closeStr) - totalWritten, 0);
					    totalWritten += charsWritten;
					}
					//close connection and exit the child process
					close(establishedConnectionFD);
					_exit(0);
				}

				//breakup the string
				char * token;
				//first calls moves internal pointer past the ^^
				//get the plaintext
				token = strtok(buffer, "$");
				strcpy(textContents, token);
				chopN(textContents, 1);
				token = strtok(NULL, "@@");
				strcpy(keyContents, token);

				memset(buffer, '\0', BUFF_SIZE);
				unsigned int i;
				int textAscii = -5;
				int keyAscii = -5;
				int totalAscii = -5;
				int textFlag = 0;
				int keyFlag = 0; 
				for(i = 0; i < strlen(textContents); i++)
				{
					//loopthrough usable chars to find text[i] and key[i]
					//use those indices for the math
					unsigned int j;
					for(j = 0; j < strlen(usableChars); j++)
					{
						//look to match text and key char
						if (textContents[i] == usableChars[j] && textFlag == 0)
						{
							//set flag anf store ascii char
							textFlag = 1;
							//store the index value as an int
							textAscii = j;
						}
						if (keyContents[i] == usableChars[j] && keyFlag == 0)
						{
							keyFlag = 1;
							keyAscii = j;
						}
						//if found both break from loop
						if (textFlag == 1 && keyFlag == 1)
						{
							break;
						}
					}
					//calculate the total and mod it.
					//then use the value to get the index and store the char
					totalAscii = (textAscii + keyAscii) % 27;
					buffer[i] = usableChars[totalAscii];
					textFlag = 0, keyFlag = 0;
					textAscii = -5, keyAscii = -5;
				}

				//buffer should contain encrypted code, add @@ and send to client
				strcat(buffer, "@@");
				totalWritten = 0;
				charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0);
				if (charsRead < 0) error("ERROR writing to socket", 1);
				totalWritten += charsWritten;
				while(totalWritten < strlen(buffer))
				{
					printf("NOT ALL CHARS SENT\n");
				    //offset what we need to send
				    charsWritten = send(establishedConnectionFD, buffer + totalWritten, strlen(buffer) - totalWritten, 0);
					if (charsRead < 0) error("ERROR writing to socket", 1);
					totalWritten += charsWritten;
				}
				//exit the child process
				close(establishedConnectionFD);
				_exit(0);
				break;
			}
			//parent
			default:
			{
				//close dupe FD
				close(establishedConnectionFD);
				break;

			}
		}
	}
	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
