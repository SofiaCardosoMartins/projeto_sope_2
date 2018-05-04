#include "macros.h"
#include "string.h"
#include "stdlib.h"

struct client_request {                  // delay in microseconds
  int timeout_ms;                       // client timeout in milliseconds
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS];       // list of preferences (seat numbers)
};

void arrayChar_to_arrayInt(char* arrayChar,int* arrayInt);
int main(int argc, char *argv[]);
