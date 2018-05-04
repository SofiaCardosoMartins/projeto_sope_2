#include "client.h"
#include "macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void arrayChar_to_arrayInt(char* arrayChar,int* arrayInt)
{
  int preferedSeatNumber = 0;
  char preferedSeat[WIDTH_SEAT] = {'\0'};
  char *answerFifoName;
  int fd_ans, fd_req;
  int ans;
  int timeout = atoi(argv[1]);

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
  rc.client_pid = getpid();
  rc.num_wanted_seats = stoi(*argv[2]);
  rc.timeout_ms = atoi(argv[1]);
  rc.num_wanted_seats = atoi(argv[2]);
  memset(rc.preferences,0,sizeof(rc.preferences));
  arrayChar_to_arrayInt(argv[3],rc.preferences);

  sprintf(answerFifoName, "ans%d", getpid());

  //creating fifo for request answers receiving
  mkfifo(answerFifoName, 0660);
  fd_ans = open(answerFifoName, O_RDONLY | O_NONBLOCK);


  //opening fifo for request sending
  do
  {
    fd_req = open("requests", O_WRONLY);
    if (fd_req == -1)
      sleep(1);
  } while (fd_req == -1);

  write(fd_req, &rc, sizeof(rc));

  //sleeping during timeout
  int rd, timeout_cnt = 0;
  do{
    rd = read(fd_ans, ans, sizeof(ans));
    usleep(1000);
    timeout_cnt++;
  } while(rd = -1 && timeout_cnt < timeout);

  return 0;
}
