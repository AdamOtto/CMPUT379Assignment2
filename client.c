#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>

#define MY_PORT 2224
#define HOSTNAME "localhost"

int main(int argc, char *argv[]) {

    char *hostname = "localhost";
    char *keyfile;
    int portnumber = MY_PORT;
    int stringSize = 128;

    printf("\n");
    if (argc == 3) {
        hostname = argv[1];
        portnumber = atoi(argv[2]);
    } else if (argc == 4) {
        hostname = argv[1];
        portnumber = atoi(argv[2]);
        keyfile = argv[3];
    } else {
        printf("Incorrect number of arguments supplied.\n");
        // exit (1);
    }

    int	s, number;
    char c[stringSize];
    struct sockaddr_in server;
    struct hostent	*host;

    host = gethostbyname(hostname);

    if (host == NULL) {
        perror ("Client: cannot get host description");
        exit (1);
    }

    s = socket (AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        perror ("Client: cannot open socket");
        exit (1);
    }

    bzero (&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons (portnumber);

    if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
        perror ("Client: cannot connect to server");
        exit (1);
    }

	recv(s,c,stringSize,0);
	printf("%s\n",c);
	
	while(1)
	{
		printf("Please type a command:\n");
		char sendStr[stringSize];	
		fgets (sendStr, stringSize, stdin);
		send(s,sendStr,stringSize,0);
		if(sendStr[0] == '@')
		{
			//Send the message next.
			fgets (sendStr, stringSize, stdin);
			send(s,sendStr,stringSize,0);
		}
		recv(s,c,stringSize,0);
		printf("%s\n",c);
	}

	close (s);
}
