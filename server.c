#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

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
int fd, id[NUM_THREADS];


int main(int argc, char * argv[]) {

	char *statefile;
	int portnumber = MY_PORT;	

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

	int	sock, snew, fromlength, number, outnum;	
	struct	sockaddr_in	master, from;
	char c[stringSize];
	//char whiteBoardMessages[entries][stringSize];
	
	int i = 0;
	
	int optval = 1;
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
	
	while(1)
	{
		while(listen (sock, 0)) {}
		fromlength = sizeof (from);
		snew = accept (sock, (struct sockaddr*) & from, & fromlength);

		if (snew < 0) {
			perror ("Server: accept failed");
			exit (1);
		}
		outnum = htonl (number);

		//Start a new thread.
		id[i] = i;
		pthread_mutex_init(&mutex,NULL);
		pthread_create(&thread[i], NULL, MessageBoard, (void *)&snew);
		i++;
	}

	close (snew);
	sleep(1);
}

void * MessageBoard(void * socket){
	int snew = *((int *)socket);

	char c[stringSize];

	sprintf(c,"CMPUT379 Whiteboard Server v0\n%d\n", entries);
	puts(c);
	
	send(snew,c,stringSize,0);

	while(1)
	{
		printf("Waiting for request.\n");
		recv(snew,c,stringSize,0);
		int index = 0;
		int * StringParseIndex = &index;
		int entryNum = 0;
		int entrylength = 0;
		int encryptedFlag = 0; //0: plaintext, 1: encrypted
		printf("request: %s\n",c);
		switch(c[0]) {
			case '?':
				printf("Received entry request\n");
				entryNum = getIntFromString(1, c, stringSize, StringParseIndex);
				
				if(c[*StringParseIndex] == 'e'){
					printf("Encypted message received\n");
					encryptedFlag = 1;
					//Decrypt
				}
				else if(c[*StringParseIndex] == 'p')
					printf("Plaintext message received\n");	
				
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
				//TODO: lock whiteBoardMessages to prevent threading conflict.
				entrylength = getStringSize(whiteBoardMessages[entryNum]);
				sprintf(c,"!%dp%d\n%s\n", entryNum, entrylength, whiteBoardMessages[entryNum]);
				send(snew, c, stringSize,0);
			break;
			case '@':
				printf("Received update request\n");
				entryNum = getIntFromString(1, c, stringSize, StringParseIndex);

				if(c[*StringParseIndex] == 'e'){
					printf("Encypted message received\n");
					encryptedFlag = 1;
					//Decrypt
				}
				else if(c[*StringParseIndex] == 'p')
					printf("Plaintext message received\n");	

				entrylength = getIntFromString( *StringParseIndex + 1, c, stringSize, StringParseIndex);
				
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
				if(entrylength >= stringSize)
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
					//Update and send success message.
					//TODO: lock whiteBoardMessages to prevent threading conflict.					
					memcpy(whiteBoardMessages[entryNum], &c, entrylength);
				}
				else {
					//TODO: lock whiteBoardMessages to prevent threading conflict.
					memset(whiteBoardMessages[entryNum], 0, stringSize);			
				}
				printf("Update Successful.\n");
				if(encryptedFlag == 1)
					sprintf(c,"!%de%d\n\n", entryNum, entrylength);
				else
					sprintf(c,"!%dp%d\n\n", entryNum, entrylength);
				puts(c);
				send(snew,c,stringSize,0);
			break;
			default:
				sprintf(c,"\nUnexpected Query. Terminating Connection.\n");
				send(snew,c,stringSize,0);
				close (snew);
				return;
			}
	}
}

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

void LoadWhiteBoard()
{
 	FILE *fp;

	fp = fopen("/home/user/whiteboard.all", "r");

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

void signalhandler(int signal) {
	int i;
	FILE *fp;
	fp = fopen("/home/user/whiteboard.all", "w");
	for(i = 0; i < entries; i++)
		fprintf(fp, "%s", whiteBoardMessages[i]);
	fclose(fp);
	
	exit(1);
}
