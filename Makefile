CC = gcc

all: client server

client: client.c
	$(CC) $< -o wbc379

server: server.c 
	$(CC) $< -o wbs379 -lpthread

clean:
	rm -f *~ *.o wbc379 wbs379
