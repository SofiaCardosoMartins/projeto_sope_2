#include "macros.h"
#include "string.h"
#include "stdlib.h"
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

struct client_request {                  // delay in microseconds
  int client_pid;                        // client's process id
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS];       // list of preferences (seat numbers)
};

struct server_answer {
  int state;  //0 - ok. !0 - error
  int seats[];  //number of reserved seats followed by the seats' numbers.
};

void write_to_clog_cbook(struct server_answer* answer, bool timeout);
void timeout_handler();
void arrayChar_to_arrayInt(char* arrayChar,int* arrayInt);
