#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "messageboard.h"

#define NUM_THREADS 10
#define MAX_STRING 128
const int strSize = MAX_STRING;

/*Global Variables*/
int entries;
char whiteBoardMessages[38][MAX_STRING];

static struct sigaction exitSignalHandler;
extern pthread_mutex_t mutex;
pthread_t thread;
char buff[PATH_MAX];

void signalhandler(int signal);
void LoadWhiteBoard();

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
                    memset(whiteBoardMessages[entryNum], 0, stringSize);			
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

int main(int argc, char * argv[]) {

    char *statefile;
    int portnumber;
    int sock, snew, fromlength, number, outnum;	
    struct sockaddr_in master, from;
    char c[strSize];	
    int optval = 1;
    char *cwd = getcwd(buff, sizeof(buff));

    if (argc == 4) {
        portnumber = atoi(argv[1]);
        if (strcmp(argv[2], "-f") == 0) {
            statefile = argv[3];
            // Load previous server content
            LoadWhiteBoard(statefile);
        } else if (strcmp(argv[2], "-n") == 0) {
            entries = atoi(argv[3]);
        } else {
            printf("Incorrect option.\n");
        }
    } else {
        printf("Incorrect number of arguments supplied.\n");
        exit(1);
    }

    // Make main() a daemon task
    // To stop the server, type "kill 'id'" where id is the pid of the child process.

    pid_t pid = fork();
    pid_t sid = 0;

    if (pid < 0) {
        printf("fork failed!\n");
        exit(1);
    }

    if (pid > 0) {
        // In the parent
        printf("pid of child process %d \n", pid);
        exit(0);
    }

    umask(0);

    // Open logs here.
    sid = setsid();
    if (sid < 0) {
        exit(1);
    }

    if ((chdir("/")) < 0) {
        printf("Could not change working directory to /\n");
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Create the signal handler to handle SIGTERM
    exitSignalHandler.sa_handler = signalhandler;	
    exitSignalHandler.sa_flags = 0;
    sigaction(SIGTERM, &exitSignalHandler, 0);

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        printf("setsockopt error");
    if (sock < 0) {
        perror("Server: cannot open master socket");
        exit(1);
    }

    master.sin_family = AF_INET;
    master.sin_addr.s_addr = inet_addr("127.0.0.1");
    master.sin_port = htons(portnumber);

    if (bind(sock, (struct sockaddr*) &master, sizeof(master))) {
        perror ("Server: cannot bind master socket");
        exit(1);
    }
	
    // Main loop for the server
    while (1) {
        // Start listening for clients
        listen (sock, 0);
        fromlength = sizeof(from);
        snew = accept(sock, (struct sockaddr*) &from, &fromlength);

        if (snew < 0) {
            perror ("Server: accept failed");
            exit (1);
        }
        outnum = htonl(number);

        // Start a new thread for the client
        pthread_mutex_init(&mutex, NULL);
        pthread_create(&thread, NULL, MessageBoard, (void *)&snew);
    }
}



/*
 * LoadWhiteBoard
 *
 * Loads the whiteboard with elements found in the state file.
 */
void LoadWhiteBoard(char *fileName) {
    FILE *fp;

    char path[PATH_MAX];
    strcpy(path, buff);
    strcat(path, "/");
    strcat(path, fileName);

    fp = fopen(path, "r");

    char c[strSize];
    if (fp) {
        int i = 0;
        while (fgets(c, sizeof c, fp) != NULL) {
            if (i < entries) {
                puts(c);
                memcpy(whiteBoardMessages[i], c, strSize);	
                i++;
            }
        }
        fclose(fp);
    } else {
        printf("Could not load file.\n");
    }
    return;
}

/*
signalhandler

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
