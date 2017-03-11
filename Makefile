CC = gcc

host = localhost
port = 2224

all: client server

client: client.c
	$(CC) $< -o wbc379

server: server.c	
	$(CC) $< -o wbs379 -lpthread

clean:
	rm -f *~ wbc379 wbs379
