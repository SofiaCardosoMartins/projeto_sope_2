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
#include "structs.h"

//
// Ancillary functions (see description along their definition)
//
static bool redirect_stdin(int fd);

static int read_client_info(struct client_info *ci);

static pid_t create_client_process(const struct client_info *ci);

static int handle_zombies(int flags);

static int main_loop();
