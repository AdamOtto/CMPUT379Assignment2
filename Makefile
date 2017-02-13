CC = gcc

all: client server

client: client.c
	$(CC) $< -o wbc379

server: server.c
	$(CC) $< -o wbs379

clean:
	rm -f *~ wbc379 wbs379
