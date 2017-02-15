#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define	MY_PORT	2224

int main(int argc, char * argv[]) {

	char *statefile;
	int entries = 38;
	int portnumber = MY_PORT;	
	int stringSize = 128;	
	
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
	char whiteBoardMessages[entries][stringSize];

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

	listen (sock, 0);
	fromlength = sizeof (from);
	snew = accept (sock, (struct sockaddr*) & from, & fromlength);
	if (snew < 0) {
		perror ("Server: accept failed");
		exit (1);
	}
	outnum = htonl (number);
	
	sprintf(c,"CMPUT379 Whiteboard Server v0\n%d\n", entries);
	puts(c);
	send(snew,c,stringSize,0);
	
	while(1)
	{
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
				send(snew, whiteBoardMessages[entryNum], stringSize,0);
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
				//Update and send success message.
				memcpy(whiteBoardMessages[entryNum], &c, entrylength);
				if(encryptedFlag == 1)
						sprintf(c,"!%de%d\n\n", entryNum, entrylength);
					else
						sprintf(c,"!%dp%d\n\n", entryNum, entrylength);
				puts(c);
				send(snew,c,stringSize,0);
			break;
		}
	}

	/*
	// Zero out all of the bytes in character array c
	bzero(c,11);

	// Here we print off the values of character array c to show that
	// each byte has an intial value of zero before receiving anything
	// from the client.
	printf("Before recieving from client\n--------------------------\n");
	printf("Character array c has the following byte values:\n");
	for (i = 0; i < 11; i++){
		printf("c[%d] = %d\n",i,c[i]);
	}

	// Now we receive from the client, we specify that we would like 11 bytes
	recv(snew,c,11,0);

	// Print off the received bytes from the client as a string. 
	// Next, print off the value of each byte to showcase that indeed
	// 11 bytes were received from the client
	printf("\nAfter receiving from client\n-------------------------\n");
	printf("Printing character array c as a string is: %s\n",c);
	printf("Character array c has the following byte values:\n");
	for (i = 0; i < 11; i++){
		printf("c[%d] = %d\n",i,c[i]);
	}

	//copy the string "Stevens" into character array c
	strncpy(c,"Stevens",7);
	
	//Send the first five bytes of character array c back to the client
	//The client, however, wants to receive 7 bytes.
	send(snew,c,5,0);
	*/
	close (snew);
	sleep(1);
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
