CC = gcc

all: client server

client: client.c
	$(CC) $< -o $@

server: server.c
	$(CC) $< -o $@

clean:
	rm -f *~ client server
