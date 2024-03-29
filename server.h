#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "structs.h"

#define SHARED 0 /* sem. is shared between threads */

void write_to_file(int fd, void* buf, size_t count);
void intdup(int const *src, int *dest, size_t size);
void *ticketOffice(void *arg);
void processRequest(int fd_slog, struct client_request* cr);
void alarmHandler(int signo);
void termHandler(int signo);
int isSeatFree(struct Seat *seats, int seatNum);
void bookSeat(struct Seat *seats, int seatNum, int clientId);
void freeSeat(struct Seat *seats, int seatNum);
int getTicketOfficeNum(pthread_t tid);
void writeRequestAnswer(int fd_slog, char *ticketOfficeNum, struct client_request* cr, struct server_answer sa);
char *getSpaceString(int n);
void termHandler(int signo);
void sigint_handler(int signo);
void destroy_mutexes();