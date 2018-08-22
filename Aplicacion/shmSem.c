//
//  shmSem.c
//
//
//  Created by Fermin Gomez on 8/22/18.
//
//

#include "shmSem.h"

char * setUpSharedMemory(int size, const char * name) {
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    char * shmAddr;
    if(shm_fd == -1 || ftruncate(shm_fd, size) == -1) {
        printf("Can't initialize shared memory");
        exit(-1);
    }
    shmAddr = mmap(0, size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    return shmAddr;
}

sem_t * createSemaphore(const char * name) {
    sem_t * sem = sem_open(name, O_CREAT, 0644, 0);
    if(sem == SEM_FAILED) {
        printf("Unable to create semaphores\n");
        endSemaphore(sem, name);
        exit(-1);
    } else {
        return sem;
    }
}

void endSemaphore(sem_t * sem, const char * name) {
    sem_close(sem);
    sem_unlink(name);
}
