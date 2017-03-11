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

int entries = 38;
const int stringSize = 128;
char whiteBoardMessages[38][128];
pthread_mutex_t mutex;

/*
 * getStringSize
 *   arguments:
 *     char stringToRead[]: the string we will operate on
 *
 *   Function returns how many characters are in a char array.
 *   Reads through it until a '\0' character is found.
 */
int getStringSize(char stringToRead[]) {
    int count = 0;
    while (1) {
        if (stringToRead[count] != '\0' )
            count++;
        else
            return count;
    }
}

/*
 * getIntFromString
 *   arguments:
 *     int startingIndex: the starting index we will start reading from
 *     char stringToRead[]: the string that will be parsed
 *     int sizeOfString: the size of the string
 *     int * parseIndex: an index storing how far we parsed into the string
 *
 *   Returns an integer that found within a string. The starting index must be where the integer starts.
 */
int getIntFromString(int startingIndex, char stringToRead[], int sizeOfString, int *parseIndex) {
    int i, j = startingIndex;
    for (i = startingIndex; i < sizeOfString; i++) {
        if ((int)stringToRead[i] >= 48 &&  (int)stringToRead[i] <= 57)
            j++;
        else
            break;
    }
    char subBuf[j];
    memcpy(subBuf, &stringToRead[startingIndex], j);
    *parseIndex = j;
    return atoi(subBuf);
}

/*
 * MessageBoard
 *   arguments:
 *     void *socket: void pointer containing the socket information
 *     int entries: number of entries
 *
 *   Function that each thread will run.  Takes inputs from a client
 *   and sends back the appropriate response.
 */
void * MessageBoard(void *socket) {

    // Unpack socket into int
    int snew = *( (int *) socket);
    char c[stringSize];

    sprintf(c, "CMPUT379 Whiteboard Server v0\n%d\n", entries);
    puts(c);

    send(snew, c, stringSize, 0);

    while (1) {
        printf("Waiting for request.\n");
        recv(snew, c, stringSize, 0);

        // Initialize some variables needed
        int encryptedFlag = 0; // 0: plaintext, 1: encrypted
        int index = 0, entryLength = 0, entryNum = 0;
        int *StringParseIndex = &index;

        printf("request: %s\n", c);

        // Handle read requests from client
        switch (c[0]) {

            case '?':
                printf("Received entry request\n");
                entryNum = getIntFromString(1, c, stringSize, StringParseIndex);
				
                // Check if message is encrypted
                if (c[*StringParseIndex] == 'e') {
                    printf("Encypted message received\n");
                    encryptedFlag = 1;
                    // Decrypt
                } else if (c[*StringParseIndex] == 'p') {
                    printf("Plaintext message received\n");
                }	
				
                // Ensure request is within bounds
                if (entryNum >= entries) {
                    if (encryptedFlag == 1) {
                        sprintf(c, "!%de%d\nEntry does not exist.\n", entryNum, entryLength);
                    } else {
                        sprintf(c, "!%dp%d\nEntry does not exist.\n", entryNum, entryLength);
                    }
                    puts(c);
                    send(snew, c, stringSize, 0);
                    break;
                }

                // Send entry to client
                pthread_mutex_lock(&mutex);
                entryLength = getStringSize(whiteBoardMessages[entryNum]);
                sprintf(c, "!%dp%d\n%s\n", entryNum, entryLength, whiteBoardMessages[entryNum]);
                pthread_mutex_unlock(&mutex);
                send(snew, c, stringSize, 0);
                break;

            // Handle update requests
            case '@':
                printf("Received update request\n");
                entryNum = getIntFromString(1, c, stringSize, StringParseIndex);

                // Check if message is encrypted
                if (c[*StringParseIndex] == 'e') {
                    printf("Encypted message received\n");
                    encryptedFlag = 1;
                    // Decrypt
                } else if (c[*StringParseIndex] == 'p') {
                    printf("Plaintext message received\n");
                }

                // Find number of characters expected in message
                entryLength = getIntFromString(*StringParseIndex + 1, c, stringSize, StringParseIndex);
				
                // Ensure request is within bounds
                if (entryNum >= entries) {
                    if (encryptedFlag == 1) {
                        sprintf(c, "!%de%d\nEntry does not exist.\n", entryNum, entryLength);
                    } else {
                        sprintf(c, "!%dp%d\nEntry does not exist.\n", entryNum, entryLength);
                    }
                    puts(c);
                    send(snew, c, stringSize, 0);
                    break;
                }
                
                // Ensure message is short enough
                if (entryLength > stringSize) {
                    if (encryptedFlag == 1) {
                        sprintf(c, "!%de%d\nMessage is too long.\n", entryNum, entryLength);
                    } else {
                        sprintf(c, "!%dp%d\nMessage is too long.\n", entryNum, entryLength);
                    }
                    puts(c);
                    send(snew, c, stringSize, 0);
                    break;
                }

                // Get message next			
                recv(snew, c, stringSize, 0);

                if (entryLength != 0) {					
                    // Update entry if length > 0
                    pthread_mutex_lock(&mutex);				
                    memcpy(whiteBoardMessages[entryNum], &c, entryLength);
                    pthread_mutex_unlock(&mutex);
                } else {
                    // Clear entry if length == 0
                    pthread_mutex_lock(&mutex);
                    memset(whiteBoardMessages[entryNum], 0, stringSize);
                    pthread_mutex_unlock(&mutex);			
                }
                printf("Update Successful.\n");
				
                // Send success message
                if (encryptedFlag == 1) {
                    sprintf(c, "!%de%d\n\n", entryNum, entryLength);
                } else {
                    sprintf(c,"!%dp%d\n\n", entryNum, entryLength);
                }
                puts(c);
                send(snew, c, stringSize, 0);
                break;

            default:
                // Terminate connection and exit if unexpected query received
                sprintf(c, "\nUnexpected Query. Terminating Connection.\n");
                send(snew, c, stringSize, 0);
                close(snew);
                return;
        }
    }
}
