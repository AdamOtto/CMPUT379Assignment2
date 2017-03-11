CC = gcc

all: client message server

client: client.c
	$(CC) $< -o wbc379

message: messageboard.c messageboard.h
	$(CC) -c $<

server: server.c messageboard.o
	$(CC) -c $<
	$(CC) server.o messageboard.o -o wbs379 -lpthread

clean:
	rm -f *~ *.o wbc379 wbs379
