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
        printf("Hostname: %s\nPort number: %d\n", hostname, portnumber);
    } else if (argc == 4) {
        hostname = argv[1];
        portnumber = atoi(argv[2]);
        keyfile = argv[3];
        printf("Hostname: %s\nPort number: %d\nKeyfile: %s\n", hostname, portnumber, keyfile);
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
		scanf("%s", sendStr);
		send(s,sendStr,stringSize,0);

		recv(s,c,stringSize,0);
		printf("%s\n",c);
	}
	/*
	// Zero out all bytes in character array c
	bzero(c,11);
	// assign 'M' to the last element of c
	c[10] = 'M';
	// assign 'A' to the first element of c
	c[0] = 'A';

	// Send all 11 bytes of character array c to the server
	// It is important to note that even the null terminating (zero valued) bytes
	// are sent to the server. 
	send(s,c,11,0);

	// zero out each byte of the array before receiving from the server
	bzero(c,11);

	// Here the client wants to receive 7 bytes from the server, but the server
	// only sends 5 bytes
	recv(s,c,7,0);
	sleep(2);

	// Now we print out the character array to reveal that only 5 bytes were
	// received.
	printf("%s\n",c);
	*/
    close (s);
}
