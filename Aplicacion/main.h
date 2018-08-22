
#ifndef MAIN_H
#define MAIN_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>

#define DEFAULTSLAVES 8
#define SHMSIZE 2000

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
void pipeSlaves(int * fd);
void generateSlaves();
int initialDistribution(const char * argv[], int dim);
void killSlaves();
void writeDataToBuffer(int fd, const void * buffer);
char * setUpSharedMemory(int size);
void createSemaphore();
void endSemaphore();
int getNumberOfCores();
void initializeArrays();
void freeArrays();

const char * shmName = "sharedMemoryViewAndApp";
const char * semViewName = "viewSemaphore";

sem_t * semView;

int numOfSlaves;

int * fdHash; // hash
int * fdFiles; // archivos

char * shmAddr; // shared memory address (buffer)

pid_t * childs;


#endif
