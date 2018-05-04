#include "client.h"
#include "macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

  printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

  if (argc != 4)
    printf("Invalid argumments\n Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");

  struct client_request rc;
  rc.timeout_ms = stoi(*argv[1]);
  rc.num_wanted_seats = stoi(*argv[2]);

  sleep(1);

  return 0;
}
