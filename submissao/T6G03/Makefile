# Makefile

all: start server client

start:	start.c
	gcc -Wall -o T6G03/start start.c

server:	server.c
	gcc -Wall -lrt -o T6G03/server server.c -lpthread

client:	client.c
	gcc -Wall -o T6G03/client client.c

clean:
	rm -rf *.o T6G03/start T6G03/server T6G03/client
