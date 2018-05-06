// uncomment this line to disable passing invalid arguments to client processes
// (or define it through command line/Makefile)
//#define ADDITIONAL_CHECK

#define MAX_ROOM_SEATS 9999             /* maximum number of room seats/tickets available       */
#define MAX_CLI_SEATS 99                /* maximum number of seats/tickets per request          */
#define WIDTH_PID 5                     /* length of the PID string                             */
#define WIDTH_XXNN 5                    /* length of the XX.NN string (reservation X out of N)  */
#define WIDTH_SEAT 4                    /* length of the seat number id string                  */
#define WIDTH_ERROR 3                   /* length of the errors in the clog.txt file -MAX,NST...*/

// maximum length of the preference list string (the +1 is for the space character)
#define MAX_PREFERENCES_LEN ((WIDTH_SEAT+1)*(MAX_CLI_SEATS))

#define MAX_TOKEN_LEN 1024              /* length of the largest string within config file      */
#define PREF_LIST_END "END"             /* terminator string for the list of seat preferences   */


// macro to quote ("stringify") a value
// [used by create_client_process() to create string format according to WIDTH_SEAT]
#define QUOTE(str) __QUOTE(str)
#define __QUOTE(str)  #str

// ANSI colors for terminal coloring
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"