# Makefile

all: start server client

start:	start.c
	gcc -Wall -o start start.c

server:	server.c
	gcc -Wall -o server server.c

client:	client.c
	gcc -Wall -o client client.c
