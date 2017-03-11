#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void * MessageBoard(void * socket, int entries) {

    int snew = *((int *)socket);
    char c[stringSize];

    sprintf(c, "CMPUT379 Whiteboard Server v0\n%d\n", entries);
    puts(c);
	
    send(snew, c, stringSize, 0);

    while (1) {

        int * StringParseIndex = &index;
        int index = 0, entryNum = 0, entrylength = 0;
        int encryptedFlag = 0; // 0: plaintext, 1: encrypted

        printf("Waiting for request.\n");
        recv(snew, c, stringSize, 0);
        printf("request: %s\n", c);

        switch (c[0]) {
            case '?':
                printf("Received entry request\n");
                entryNum = getIntFromString(1, c, stringSize, StringParseIndex);
				
                if (c[*StringParseIndex] == 'e') {
                    printf("Encypted message received\n");
                    encryptedFlag = 1;
                    //Decrypt
                }
                else if(c[*StringParseIndex] == 'p') {
                    printf("Plaintext message received\n");
                }	
				
                if(entryNum >= entries) {
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

int getStringSize(char stringToRead[]) {
    int count = 0;
    while(1) {
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
