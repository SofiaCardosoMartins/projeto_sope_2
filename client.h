#include "macros.h"

struct client_request {                  // delay in microseconds
  int client_pid;                        // client's process id
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS];       // list of preferences (seat numbers)
};
