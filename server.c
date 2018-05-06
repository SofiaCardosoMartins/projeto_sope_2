#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>


void * ticketOffice(void * arg){

    return NULL;
}


int main(int argc, char *argv[])
{
if (argc != 4)
    printf("Invalid argumments\n Usage: server <num_room_seats> <num_ticket_offices> <open_time> \n");

int num_room_seats = atoi(argv[0]);
int num_ticket_offices = atoi(argv[1]);
int open_time = atoi(argv[2]);
pthread_t threads[num_ticket_offices];

int fd_req;

mkfifo("requests", 0660);
fd_req = open("requests", O_RDONLY);


for(int i = 0; i < num_ticket_offices; i++){
    pthread_create(&threads[i], NULL, ticketOffice, NULL);
}


close(fd_req);          //close the fifo
unlink("requests");     //delete the fifo
free("requests");
return 0;
}
