#include "view.h"
#include "shmSem.h"
int main(int argc, const char * argv[]) {
    char aux[8];
    read(STDIN_FILENO, aux, 8);
    int appPID = atoi(aux);
    sem_t * sem = createSemaphore(semName);
    char * shmAddr = setUpSharedMemory(SHMSIZE, shmName);
    int appIsRunning = 1;
    int k = 0;
    if (kill(appPID, 0) == -1) {
        return 1;
    }
    while (appIsRunning) {
        sem_wait(sem);
        while(*(shmAddr+k) != EOF && *(shmAddr+k) != '\0') {
            if(*(shmAddr+k) != '\0')
                putchar(*(shmAddr+k));
            k++;
        }
        int semValue;
        sem_getvalue(sem, &semValue);
        if(*(shmAddr+k) == EOF || (kill(appPID, 0) == -1 && semValue == 0))
            appIsRunning = 0;
        k++;
    }
    
    munmap(shmAddr, SHMSIZE);
    shm_unlink(shmName);
    endSemaphore(sem, semName);
    return 0;
}

