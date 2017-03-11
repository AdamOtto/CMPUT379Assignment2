#include <pthread.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "functions.c"

#define	MY_PORT	2224
#define NUM_THREADS 10

void * MessageBoard(void * socket);
void signalhandler(int signal);
void LoadWhiteBoard();

/*Global Variables*/
int entries = 38;
const int stringSize = 128;
char whiteBoardMessages[38][128];
static struct sigaction exitSignalHandler;
pthread_mutex_t mutex;
pthread_t thread[NUM_THREADS];
char buff[PATH_MAX];

int main(int argc, char * argv[]) {

	char *statefile;
	int portnumber = MY_PORT;	
	int i = 0;
	int	sock, snew, fromlength, number, outnum;	
	struct	sockaddr_in	master, from;
	char c[stringSize];	
	int optval = 1;
	char *cwd = getcwd(buff, sizeof(buff));

	//Make main() a daemon task.===================================
	//To stop the server, type "kill 'id'" where id is the pid of the child process.

	pid_t pid = fork();
	pid_t sid = 0;
	
	if (pid < 0)
	{
		printf("fork failed!\n");
		exit(1);
	}

	if (pid > 0)
	{
		//in the parent
		printf("pid of child process %d \n", pid);
		exit(0);
	}

	umask(0);
	
	//Open logs here.

	sid = setsid();
	if(sid < 0)
	{
		//fprintf(fp, "cannot create new process group");
		exit(1);
	}

	if ((chdir("/")) < 0) {
		printf("Could not change working directory to /\n");
		exit(1);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	//=============================================================
	
	//Create the signal handler to handle SIGTERM
	exitSignalHandler.sa_handler = signalhandler;	
	exitSignalHandler.sa_flags = 0;
	sigaction(SIGTERM, &exitSignalHandler, 0);
	
	//Load previous server content
	LoadWhiteBoard();

	if (argc == 4) {
		portnumber = atoi(argv[1]);
		if (strcmp(argv[2], "-f") == 0) {
			statefile = argv[3];
		} else if (strcmp(argv[2], "-n") == 0) {
			entries = atoi(argv[3]);
		} else {
			printf("Incorrect option.\n");
		}
	} else {
		printf("Incorrect number of arguments supplied.\n");
		// return -1;
	}

	sock = socket (AF_INET, SOCK_STREAM, 0);
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		printf("setsockopt error");
	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = inet_addr("127.0.0.1");
	master.sin_port = htons (MY_PORT);

	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}
	
	i = 0;
	//main loop for the server.
	while(1)
	{
		//Start listening for clients.
		listen (sock, 0);
		fromlength = sizeof (from);
		snew = accept (sock, (struct sockaddr*) & from, & fromlength);

		if (snew < 0) {
			perror ("Server: accept failed");
			exit (1);
		}
		outnum = htonl (number);

		//Start a new thread for the client.
		pthread_mutex_init(&mutex,NULL);
		pthread_create(&thread[i], NULL, MessageBoard, (void *)&snew);
		i++;
	}
}


/*
MessageBoard

arguments:
	void * socket: void pointer containing the socket information.

function that each thread will run.  Takes inputs from a client
and sends back the appropriate response.
*/
void * MessageBoard(void * socket){
	//Unpack the socket into an int.
	int snew = *((int *)socket);

	char c[stringSize];

	sprintf(c,"CMPUT379 Whiteboard Server v0\n%d\n", entries);
	puts(c);
	
	send(snew,c,stringSize,0);

	while(1)
	{
		printf("Waiting for request.\n");
		recv(snew,c,stringSize,0);

		//Initialize some variables needed.
		int index = 0;
		int * StringParseIndex = &index;
		int entryNum = 0;
		int entrylength = 0;
		int encryptedFlag = 0; //0: plaintext, 1: encrypted
		printf("request: %s\n",c);
		switch(c[0]) {
			//Handles read requests from the client.
			case '?':
				printf("Received entry request\n");
				entryNum = getIntFromString(1, c, stringSize, StringParseIndex);
				
				//Checks if the message is encrypted.
				if(c[*StringParseIndex] == 'e'){
					printf("Encypted message received\n");
					encryptedFlag = 1;
					//Decrypt
				}
				else if(c[*StringParseIndex] == 'p')
					printf("Plaintext message received\n");	
				
				//Ensures the request is within bounds.
				if(entryNum >= entries)
				{
					if(encryptedFlag == 1)
						sprintf(c,"!%de%d\nEntry does not exist.\n", entryNum, entrylength);
					else
						sprintf(c,"!%dp%d\nEntry does not exist.\n", entryNum, entrylength);
					puts(c);
					send(snew,c,stringSize,0);
					break;
				}
				//Sends the entry to the client.
				pthread_mutex_lock(&mutex);
				entrylength = getStringSize(whiteBoardMessages[entryNum]);
				sprintf(c,"!%dp%d\n%s\n", entryNum, entrylength, whiteBoardMessages[entryNum]);
				pthread_mutex_unlock(&mutex);
				send(snew, c, stringSize,0);
			break;

			//Handles update requests.
			case '@':
				printf("Received update request\n");
				entryNum = getIntFromString(1, c, stringSize, StringParseIndex);

				//Checks if the message is encrypted.
				if(c[*StringParseIndex] == 'e'){
					printf("Encypted message received\n");
					encryptedFlag = 1;
					//Decrypt
				}
				else if(c[*StringParseIndex] == 'p')
					printf("Plaintext message received\n");	
				//Find out how many characters are in expected in the message.
				entrylength = getIntFromString( *StringParseIndex + 1, c, stringSize, StringParseIndex);
				
				//Ensures the request is within bounds.
				if(entryNum >= entries)
				{
					if(encryptedFlag == 1)
						sprintf(c,"!%de%d\nEntry does not exist.\n", entryNum, entrylength);
					else
						sprintf(c,"!%dp%d\nEntry does not exist.\n", entryNum, entrylength);
					puts(c);
					send(snew,c,stringSize,0);
					break;
				}
				//Ensures the message is short enough.
				if(entrylength > stringSize)
				{
					if(encryptedFlag == 1)
						sprintf(c,"!%de%d\nMessage is too long.\n", entryNum, entrylength);
					else
						sprintf(c,"!%dp%d\nMessage is too long.\n", entryNum, entrylength);
					puts(c);
					send(snew,c,stringSize,0);
					break;
				}

				//Bulletproofing done, get message next.			
				recv(snew,c,stringSize,0);

				if(entrylength != 0) {					
					//Update the entry if length is greater than 0.
					pthread_mutex_lock(&mutex);				
					memcpy(whiteBoardMessages[entryNum], &c, entrylength);
					pthread_mutex_unlock(&mutex);
				}
				else {
					//clear the entry if the entry length is 0.
					pthread_mutex_lock(&mutex);
					memset(whiteBoardMessages[entryNum], 0, stringSize);
					pthread_mutex_unlock(&mutex);			
				}

				printf("Update Successful.\n");
				
				//Send a success message.
				if(encryptedFlag == 1)
					sprintf(c,"!%de%d\n\n", entryNum, entrylength);
				else
					sprintf(c,"!%dp%d\n\n", entryNum, entrylength);
				puts(c);
				send(snew,c,stringSize,0);
			break;
			default:
				//If an unexpected query is received, terminate the connection and exit.
				sprintf(c,"\nUnexpected Query. Terminating Connection.\n");
				send(snew,c,stringSize,0);
				close (snew);
				return;
			}
	}
}

/*
getStringSize

arguments:
	char stringToRead[]: The string we will operate on.

function that returns how many characters are in a char array.
Reads through it until a '\0' character is found.
*/
int getStringSize(char stringToRead[])
{
	int count = 0;
	while(1)
	{
		if( stringToRead[count] != '\0' )
			count++;
		else
			return count;
	}
}

/*
getIntFromString

arguments:
	int startingIndex: the starting index we will start reading from.
	char stringToRead[]: the string that will be parsed.
	int sizeOfString: the size of the string.
	int * parseIndex: an index storing how far we parsed into the string.

Returns an integer that is found within a string.  The starting index must be where the integer starts.
*/
int getIntFromString(int startingIndex, char stringToRead[], int sizeOfString, int * parseIndex) {
	int i, j = startingIndex;
	for(i = startingIndex; i < sizeOfString; i++)
	{
		if ((int)stringToRead[i] >= 48 &&  (int)stringToRead[i] <= 57) {
			j++;
		}
		else
			break;
	}
	char subBuf[j];
	memcpy(subBuf, &stringToRead[startingIndex], j);
	*parseIndex = j;
	return atoi(subBuf);
}

/*
LoadWhiteBoard

arguments:

Loads the whiteboard with elements found in the whiteboard.all file.
*/
void LoadWhiteBoard()
{
 	FILE *fp;

        char path[PATH_MAX];
        strcpy(path, buff);
        strcat(path, "/whiteboard.all");

	fp = fopen(path, "r");

	char c [stringSize];
	if(fp)
	{
		int i = 0;
		while(fgets(c, sizeof c, fp) != NULL)
		{
			if (i < entries)
			{
				puts(c);
				memcpy(whiteBoardMessages[i], c, stringSize);	
				i++;
			}
		}
		fclose(fp);
	}
	else
	{
		printf("Could not load whiteboard.all file.\n");
	}
	
	return;
}

/*
LoadWhiteBoard

arguments:
	int signal: an integer representing the signal recieved.

Dumps all the content of whiteBoardMessages into a file called whiteboard.all.
*/
void signalhandler(int signal) {
    FILE *fp;
    char *mode = "w";

    char path[PATH_MAX];
    strcpy(path, buff);
    strcat(path, "/whiteboard.all");

    fp = fopen(path, mode);

    int i;
    for (i = 0; i < entries; i++) {
        fprintf(fp, "%s\n", whiteBoardMessages[i]);
    }

    fclose(fp);
    exit(1);
}
