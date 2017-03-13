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
#include "messageboard.h"

#define	MY_PORT	2224
#define NUM_THREADS 10

void signalhandler(int signal);
void LoadWhiteBoard();

/*Global Variables*/
int _entries = 38;
const int strSize = 128;
char whiteBoardMessages[38][128];
static struct sigaction exitSignalHandler;
extern pthread_mutex_t mutex;
pthread_t thread;
char buff[PATH_MAX];

int main(int argc, char * argv[]) {

	char *statefile;
	int portnumber = MY_PORT;	
	int i = 0;
	int sock, snew, fromlength, number, outnum;	
	struct	sockaddr_in	master, from;
	char c[strSize];	
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
			_entries = atoi(argv[3]);
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
		pthread_create(&thread, NULL, MessageBoard, (void *)&snew);
	}
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

	char c[strSize];
	if(fp)
	{
		int i = 0;
		while(fgets(c, sizeof c, fp) != NULL)
		{
			if (i < _entries)
			{
				puts(c);
				memcpy(whiteBoardMessages[i], c, strSize);	
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
    for (i = 0; i < _entries; i++) {
        fprintf(fp, "%s\n", whiteBoardMessages[i]);
    }

    fclose(fp);
    exit(1);
}
