#ifndef VIEW_H
#define VIEW_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#define SHMSIZE 2000


const char * shmName = "sharedMemoryViewAndApp";
const char * semViewName = "viewSemaphore";

sem_t * semView;
int shm_fd;
char * shmAddr;

void endSemaphore();
void setUpSharedMemory();
void createSemaphore();



#endif
