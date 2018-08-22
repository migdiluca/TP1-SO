#ifndef VIEW_H
#define VIEW_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

const char * shmName = "sharedMemoryViewAndApp";
const char * semName = "semaphore";

#endif
