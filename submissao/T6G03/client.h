#include "macros.h"
#include "structs.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

void write_to_file(int fd, void* buf, size_t count);
void write_to_clog_cbook(struct server_answer* answer, bool timeout);
void timeout_handler();
void arrayChar_to_arrayInt(char* arrayChar,int* arrayInt);
