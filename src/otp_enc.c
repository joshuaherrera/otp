#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

/*REVIEW ERROR EXIT STAT*/
#define BUFF_SIZE 		262144

char buffer[BUFF_SIZE];
char textContents[BUFF_SIZE];
char keyContents[BUFF_SIZE];

void error(const char *msg, int exitStat) { perror(msg); exit(exitStat); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, totalWritten, charsRead, totalRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	//use global value in program
	//char buffer[256];
	// example execution
    // otp_enc plaintext1 myshortkey 57171
	if (argc < 4) { fprintf(stderr,"USAGE: %s [plaintext file] [key] [port]\n", argv[0]); exit(1); } // Check usage & args

	//store plaintext and keys filename
	char textFileName[256];
	char keyFileName[256];
	strcpy(textFileName, argv[1]);
	strcpy(keyFileName, argv[2]);

	memset(buffer, '\0', BUFF_SIZE);

	//create file pointer to open the desired files
	FILE* fp;
	int filenum;
	for (filenum = 0; filenum < 2; filenum++)
	{
		if ((fp = fopen(argv[filenum+1], "r")) == NULL)
		{
			fprintf(stderr, "%s: can't open %s\n",argv[0], argv[filenum+1] );
			exit(1);
		}
		//read the files
		else
		{
			while(fgets(buffer, BUFF_SIZE, fp) != NULL)
			{
			    continue;
			}
			fclose(fp);
			//check for bad chars
			unsigned int i;
			for (i = 0; i < strlen(buffer); i++)
			{
				//$*!(#*djs8301these-are-all-bad-characters
				if (buffer[i] == '$' || buffer[i] == '*'
					|| buffer[i] == '!'
					|| buffer[i] == '('
					|| buffer[i] == '#'
					|| buffer[i] == '-'
					)
				{
					fprintf(stderr, "%s error: input contains bad characters\n", argv[0]);
					exit(1);
				}
			}
		}
		//clear the buffer for the next go around
		memset(buffer, '\0', BUFF_SIZE);
	}

	//now we actually copy the strings to the buffer because the contents are ok
	memset(textContents, '\0', BUFF_SIZE);
	memset(keyContents, '\0', BUFF_SIZE);
	if ((fp = fopen(textFileName, "r")) == NULL)
	{
		fprintf(stderr, "%s: can't open %s\n",argv[0], textFileName );
		exit(1);
	}
	else
	{
		while(fgets(textContents, BUFF_SIZE, fp) != NULL)
		{
		    continue;
		}
	}
	//read the key
	fclose(fp);
	if ((fp = fopen(keyFileName, "r")) == NULL)
	{
		fprintf(stderr, "%s: can't open %s\n",argv[0], keyFileName );
		exit(1);
	}
	else
	{
		while(fgets(keyContents, BUFF_SIZE, fp) != NULL)
		{
		    continue;
		}
	}

	//remove newlines from key and text
	textContents[strcspn(textContents, "\n")] = '\0';
	keyContents[strcspn(keyContents, "\n")] = '\0';

	//check length to make sure we can use the key with the plaintext
	int keysize = strlen(keyContents);
	int textsize = strlen(textContents);
	if (keysize < textsize)
	{
		fprintf(stderr, "Error: key '%s' is too short\n", keyFileName);
		exit(1);
	}

	//build up the contents into one large file
	strcpy(buffer, "^");
	strcat(buffer, textContents);
	//$$ signifies delimiter between text and key
	strcat(buffer, "$");
	strcat(buffer, keyContents);
	strcat(buffer, "@@");

	// Set up the server address struct
	//can make a fcn, pass in pointers, dont dereference server for memset, already an address
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address; local host for program
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(2); }
	memcpy( (char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length ); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket", 1);
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
	{
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		exit(2);
	}

	// Send message to server
	//this is where we pass the file to get encrypted.
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	totalWritten = charsWritten;
	if (totalWritten < 0) error("CLIENT: ERROR writing to socket", 1);
	//need this check in a loop to make sure the message is sent
	while (totalWritten < strlen(buffer)) 
		{
			printf("NOT ALL CHARS WRITTEN!\n");
			//use totalWritten to offset the buffer, and adjust how many bytes to send
			charsWritten = send(socketFD, buffer + totalWritten, strlen(buffer) - totalWritten, 0);
			totalWritten += charsWritten;
		}
	// Get return message from server
	// get the encrypted message and spew to screen
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	
	totalRead = 0;
	while(strstr(buffer, "@@") == NULL)
	{
	    //loop with the receive, adjusting the buffer pos each call
		charsRead = recv(socketFD, buffer + totalRead, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
		if (charsRead < 0) error("CLIENT: ERROR reading from socket", 2);
		totalRead += charsRead;
	}
	//get address of @@
	int endLoc = strstr(buffer, "@@") - buffer;
	buffer[endLoc] = '\0';
	if (strcmp(buffer, "~~") == 0)
	{
		//connected to wrong server!
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		exit(2);
	}
	printf("%s\n", buffer);

	close(socketFD); // Close the socket
	return 0;
}
