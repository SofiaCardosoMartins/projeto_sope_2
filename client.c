#include "client.h"
#include "macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void arrayChar_to_arrayInt(char* arrayChar,int* arrayInt)
{
  int preferedSeatNumber = 0;
  char preferedSeat[WIDTH_SEAT] = {'\0'};

  for(int i =0;i<=strlen(arrayChar);i++)
  {
    if((arrayChar[i] != ' ') && (arrayChar[i] != '\0'))
      sprintf(preferedSeat,"%s%c",preferedSeat,arrayChar[i]);
    else
    {
      arrayInt[preferedSeatNumber] = atoi(preferedSeat);
      preferedSeatNumber++;
      preferedSeat[0] = '\0';
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 4)
    printf("Invalid argumments\n Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");

  struct client_request rc;
  rc.timeout_ms = atoi(argv[1]);
  rc.num_wanted_seats = atoi(argv[2]);
  memset(rc.preferences,0,sizeof(rc.preferences));
  arrayChar_to_arrayInt(argv[3],rc.preferences);

  return 0;
}
