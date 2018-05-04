// standard C libraries
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// UNIX/Linux libraries
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

//Other libraries
#include "macros.h"

/**
 * @brief Enumeration to increase the readability of exit status.
 * 
 */
enum TP2_Exit_Status {
  INVALID_CMD_ARGS = 1,                 // the set of arguments is invalid
  FILE_OPEN_FAILED,                     // unable to open configuration file
  SIG_HANDLER_ERR,                      // unable to install signal handler
  FORK_FAILED,                          // unable to create a new process
  SET_PGID_FAILED,                      // unable to set process group
  STDIN_REDIR_ERR,                      // unable to redirect standard input
  CLIENT_RUNTIME_ERR,                   // one or more client processes failed (exit status != 0)
  CLIENT_EXEC_FAILED                    // unable to execute client program (execlp call failed)
};

/**
 * @brief Enumeration to increase the readability of read_client_info() function's return status.
 * 
 */
enum ReadClientInfoStatus {
  INPUT_ENDED = 0,                      // EOF reached
  INVALID_POINTER = -11,                // pointer to struct client_info object is NULL
  INVALID_START_DELAY = -12,            // unable to read/parse the start delay
  INVALID_NUM_WANTED_SEATS = -13,       // unable to read/parse the number of seats/tickets wanted
  INVALID_SEAT_NUMBER = -14,            // unable to read/parse a seat number (list of preferences)
  LIST_PREF_TOO_SHORT = -15,            // the list of preferences is too short (< #seats/tickets)
  INVALID_TIMEOUT = -16                 // unable to read/parse client timeout
};

/**
 * @brief Structure to hold the required information about a given client.
 * 
 *            seq_no: relative order number within the file (useful for error messages);
 *          delay_us: the time offset, in microseconds and relative to the previous client, when the
 *                    client should run [before invoking fork()];
 *           timeout: client timeout in milliseconds;
 *  num_wanted_seats: number of seats/tickets requested by the client;
 *       preferences: the list of preferences (seat numbers).
 */
struct client_info {
  int seq_no;                           // sequence number (client relative order)
  int delay_us;                         // delay in microseconds
  int timeout_ms;                       // client timeout in milliseconds
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS];       // list of preferences (seat numbers)
};

//
// Ancillary functions (see description along their definition)
//
static bool redirect_stdin(int fd);

static int read_client_info(struct client_info *ci);

static pid_t create_client_process(const struct client_info *ci);

static int handle_zombies(int flags);

static int main_loop();
