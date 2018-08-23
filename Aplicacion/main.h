
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
#include <sys/select.h>

#define DEFAULTSLAVES 8

void pipeSlaves(int * fd);
void generateSlaves();
int initialDistribution(const char * argv[], int dim);
void killSlaves();
int getNumberOfCores();
void initializeArrays();
void freeArrays();
int isAFile(const char *path);

const char * shmName = "sharedMemoryViewAndApp";
const char * semName = "semaphore";

int numOfSlaves;
int * fdHash;
int * fdFiles;

sem_t * sem;
char * shmAddr;

pid_t * childs;


#endif
