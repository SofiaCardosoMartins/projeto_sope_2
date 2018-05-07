#include "macros.h"

struct client_request {                  // delay in microseconds
  int client_pid;                        // client's process id
  int num_wanted_seats;                 // number of seats/tickets wanted
  int preferences[MAX_CLI_SEATS];       // list of preferences (seat numbers)
};

struct server_answer {
  int state;  //0 - ok. !0 - error
  int * seats;  //number of reserved seats followed by the seats' numbers.
};

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

struct Seat
{
    int num;
    pid_t client;
};