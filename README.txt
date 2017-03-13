**************************
CMPUT379 - Assignment 2  *
                         *
group36                  *
Justin Stuehmer          *
Adam Otto                *
**************************

Current build of Assignment 2 server allows clients
to connect to a daemonized process and update the
whiteboard with plaintext messages.

The server and client at the moment don't allow for
encrypted messages and the server has a fixed
whiteboard size. (n flag must = 38 when running
wbs379)

Whiteboard contents get dumped to whiteboard.all
upon termination.


$ make
gcc client.c -o wbc379
gcc server.c -o wbs379 -lpthread

Server Examples
$ ./wbs379 2224 -n 38
pid of child process 24164 
$ kill 24164

$ ./wbs379 2224 -f whiteboard.all
pid of child process 24142 
$ kill 24142

Client Example
./wbc379 localhost 2224
