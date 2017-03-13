#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MY_PORT 2224
#define HOSTNAME "localhost"
#define SCREEN_HEIGHT 25

//Global constants
int stringSize = 128;

void clearScreen() {
    int i;
    for (i = 0; i < SCREEN_HEIGHT; i++ )
    putchar ( '\n' );
    return;
}

void getEntry(int soc) {
    char optionStr[stringSize];
    char sendStr[stringSize];

    // Clear sendStr
    memset(sendStr, 0, stringSize);

    printf("Which Entry would you like to read?: ");
    fgets(optionStr, stringSize, stdin);
    strcat(sendStr, "?");
    strcat(sendStr, optionStr);
    strcat(sendStr, "\n");
    printf("\n");
    send(soc, sendStr, stringSize, 0);
    recv(soc, sendStr, stringSize, 0);
    printf("%s\n", sendStr);
    return;
}

void sendMessage(int soc) {
    char optionStr[stringSize];
    char sendStr[stringSize];
    char buf[5];
    // Clear sendStr
    memset(sendStr, 0, stringSize);

    printf("Which Entry would you like to write to?: ");
    fgets(optionStr, stringSize, stdin);
    optionStr[strlen(optionStr) - 1] = '\0';
    strcat(sendStr, "@");
    strcat(sendStr, optionStr);
    strcat(sendStr, "p");
    printf("Please type your message:\n");
    fgets(optionStr, stringSize, stdin);
    sprintf(buf, "%d", getMessageSize(optionStr));
    strcat(sendStr, buf);
    strcat(sendStr, "\n");
    send(soc, sendStr, stringSize, 0);
    //if (optionStr[0] == '\n')
    //printf("\n%s", sendStr);
    send(soc,optionStr,stringSize,0);
    recv(soc,sendStr,stringSize,0);
    printf("\n%s", sendStr);
    return;
}

int getMessageSize(char message[]) {
    int count = 0;
    while (1) {
        if (message[count] != '\n')
            count++;
        else
            return count;
    }
}

void sendEncryptedMessage(int soc) {
    printf("Encrypted Messages are not implemented yet.\n\n");
    return;
}

int main(int argc, char *argv[]) {

    char *hostname = "localhost";
    char *keyfile;
    int portnumber = MY_PORT;

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
    struct hostent *host;

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
	
    clearScreen();
    recv(s, c, stringSize, 0);
    printf("%s\n",c);
    char optionStr[stringSize];
    char sendStr[stringSize];

    while(1) {
        printf("Welcome to the WHITEBOARD SERVER OF THE FUTURE!\nAll messages are 128 characters long.\n");
        printf("Please select one of the following options (type in the associated number)\n");
        printf("1 - Read an entry.\n2 - Write an unencrypted message.\n3 - Write an encrypted message.\n4 - Exit.\n");
        fgets(optionStr, stringSize, stdin);
        if (optionStr[0] == '1')
            getEntry(s);
        else if (optionStr[0] == '2')
            sendMessage(s);
        else if (optionStr[0] == '3')
            sendEncryptedMessage(s);
        else if (optionStr[0] == '4') {
            send(s, "bye bye", stringSize, 0);
            break;
        }
    }

    close(s);
}
