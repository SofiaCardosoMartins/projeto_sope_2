#include "client.h"
#include "macros.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//Exit values:
//0 - ok
//1 - timeout
//2 - could not open clog.txt for writing
//3 - could not open cbook.txt for writing

char *answerFifoName;
int fd_ans;

void write_to_clog_cbook(struct server_answer *answer, bool timeout)
{
  int fd_clog = open("clog.txt", O_WRONLY | O_APPEND | O_CREAT, 0660);
  int fd_cbook = open("cbook.txt", O_WRONLY | O_APPEND | O_CREAT, 0660);
  if (fd_clog == -1)
  {
    perror("Could not open clog.txt for writing");
    exit(2);
  }
  else if (fd_cbook == -1)
  {
    perror("Could not open clog.txt for writing");
    exit(3);
  }

  //1. Write pid
  pid_t clientPid = getpid();
  char *pidString = (char *)(malloc(sizeof(char) * (1 + WIDTH_PID)));
  const char format[] =  {'%', '0', WIDTH_PID + '0', 'd', ' '};
  sprintf(pidString, format, clientPid);

  if (timeout)
  {
    write(fd_clog, pidString, sizeof(char) * strlen(pidString));
    write(fd_clog, "OUT\n", sizeof(char) * strlen("OUT\n"));
  }
  else if (answer->state == 0) //successful reservation
  {
    //2. Write number of reserved seats
    int number_seats = answer->seats[0];
    char *number_seats_string = (char *)(malloc(sizeof(char) * (2 + 1)));
    if (number_seats < 10)
      sprintf(number_seats_string, "0%d", number_seats);
    else
      sprintf(number_seats_string, "%d", number_seats);

    //3. Write reserved seats
    char *seatNumber = (char *)(malloc(sizeof(char) * (1 + 1 + WIDTH_XXNN))); //XX.NN , +1 for ' ' and '\0'
    char *seatPlace = (char *)(malloc(sizeof(char) * (1 + WIDTH_SEAT)));      //seat number
    char formatSeat[] = {'%', '0', WIDTH_SEAT + '0', 'd', ' '};
    for (int i = 1; i <= number_seats; i++)
    {
      if (i < 10)
        sprintf(seatNumber, "0%d.%s ", i, number_seats_string);
      else
        sprintf(seatNumber, "%d.%s ", i, number_seats_string);

      sprintf(seatPlace, formatSeat, answer->seats[i]);
      write(fd_clog, seatNumber, sizeof(seatNumber));
      write(fd_clog, seatPlace, sizeof(seatPlace));
      write(fd_clog, "\n", sizeof("\n"));
      write(fd_cbook, seatPlace, sizeof(seatPlace));
      write(fd_cbook, "\n", sizeof("\n"));

      seatNumber = "\0";
      seatPlace = "\0";
    }
    free(number_seats_string);
    free(seatNumber);
    free(seatPlace);
  }
  else
  {
    write(fd_clog, pidString, sizeof(pidString));
    char *errorString = malloc(sizeof(char) * WIDTH_ERROR);
    switch (answer->state)
    {
    case -1:
      sprintf(errorString, "%s", "MAX");
      break;
    case -2:
      sprintf(errorString, "%s", "NST");
      break;
    case -3:
      sprintf(errorString, "%s", "IID");
      break;
    case -4:
      sprintf(errorString, "%s", "ERR");
      break;
    case -5:
      sprintf(errorString, "%s", "NAV");
      break;
    case -6:
      sprintf(errorString, "%s", "FUL");
      break;
    default:
      break;

      write(fd_clog, errorString, sizeof(errorString));
      write(fd_clog, "\n", sizeof("\n"));
      free(errorString);
    }
  }
  free(pidString);
  close(fd_clog);
  close(fd_cbook);
  return;
}

void timeout_handler()
{
  printf("%d: entrei no timeout handler\n", getpid());

  close(fd_ans);          //close the fifo
  unlink(answerFifoName); //delete the fifo
  write_to_clog_cbook(NULL, true);
  exit(1);
}

void arrayChar_to_arrayInt(char *arrayChar, int *arrayInt)
{
  int preferedSeatNumber = 0;
  char preferedSeat[WIDTH_SEAT] = {'\0'};

  for (int i = 0; i <= strlen(arrayChar); i++)
  {
    if ((arrayChar[i] != ' ') && (arrayChar[i] != '\0'))
      sprintf(preferedSeat, "%s%c", preferedSeat, arrayChar[i]);
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
  answerFifoName = (char *)(malloc(sizeof(char) * WIDTH_PID));
  int fd_req;
  struct server_answer ans;
  int timeout = atoi(argv[1]);

  if (argc != 4)
    printf("Invalid argumments\n Usage: client <time_out> <num_wanted_seats> <pref_seat_list> \n");

  struct client_request rc;
  rc.client_pid = getpid();
  rc.num_wanted_seats = atoi(argv[2]);
  memset(rc.preferences, 0, sizeof(rc.preferences));
  arrayChar_to_arrayInt(argv[3], rc.preferences);

  sprintf(answerFifoName, "ans%d", getpid());

  //creating fifo for request answers receiving
  mkfifo(answerFifoName, 0660);
  fd_ans = open(answerFifoName, O_RDONLY | O_NONBLOCK);

  //opening fifo for request sending
  /*
  do
  {
    fd_req = open("requests", O_WRONLY);
    if (fd_req == -1)
      sleep(1);
  } while (fd_req == -1);*/

  // write(fd_req, &rc, sizeof(rc));
  // close(fd_req);

  //dealing with the timeout
  int rd, timeout_cnt = 0;
  do
  {
    rd = read(fd_ans, &ans, sizeof(ans));
    usleep(1000);
    timeout_cnt++;
  } while ((rd = -1) && (timeout_cnt < timeout));

  if (timeout_cnt >= timeout)
    timeout_handler();

  write_to_clog_cbook(&ans, false);
  close(fd_ans);          //close the fifo
  unlink(answerFifoName); //delete the fifo
  free(answerFifoName);

  return 0;
}
