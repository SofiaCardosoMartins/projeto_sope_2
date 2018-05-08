# Makefile

all: start server client

start:	start.c
	gcc -Wall -o start start.c

server:	server.c
	gcc -Wall -lrt -o server server.c -lpthread

client:	client.c
	gcc -Wall -o client client.c

clean:
	rm -rf *.o
