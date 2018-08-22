//
//  shmSem.h
//
//
//  Created by Fermin Gomez on 8/22/18.
//
//

#ifndef shmSem_h
#define shmSem_h
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHMSIZE 2000

char * setUpSharedMemory(int size,const char * name);
sem_t * createSemaphore(const char * name);
void endSemaphore(sem_t * sem, const char * name);

#endif /* shmSem_h */
