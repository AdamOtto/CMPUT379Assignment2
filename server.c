#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define	MY_PORT	2224
#define NUM_THREADS 10

void * MessageBoard(void * socket);

/*Global Variables*/
int entries = 38;
const int stringSize = 128;
char whiteBoardMessages[38][128];

pthread_mutex_t mutex;
pthread_t thread[NUM_THREADS];
int fd, id[NUM_THREADS];


int main(int argc, char * argv[]) {

	char *statefile;
	//int entries = 38;
	int portnumber = MY_PORT;	
	//int stringSize = 128;	
	
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
		pthread_create(&thread[i], NULL, MessageBoard, (void *) snew);
		i++;
	}
	
	
	/*
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
					//TODO: bullet proofing
					if(encryptedFlag == 1)
						sprintf(c,"!%de%d\nEntry does not exist.\n", entryNum, entrylength);
					else
						sprintf(c,"!%dp%d\nEntry does not exist.\n", entryNum, entrylength);
					puts(c);
					send(snew,c,stringSize,0);
					break;
				}
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
					memcpy(whiteBoardMessages[entryNum], &c, entrylength);
				}
				else {
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
				sprintf(c,"\nUnexpected Query.\n");
				send(snew,c,stringSize,0);
		}
	}
	*/
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
					memcpy(whiteBoardMessages[entryNum], &c, entrylength);
				}
				else {
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
				sprintf(c,"\nUnexpected Query.\n");
				send(snew,c,stringSize,0);
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
	//printf("%d\n",sizeOfString);
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
	//printf("subBuf: %s\n", subBuf);
	return atoi(subBuf);
}
