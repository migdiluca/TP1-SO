#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
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

int main(int argc, const char * argv[]) {
    //Getting app PID
    char aux[8];
    read(STDIN_FILENO, aux, 8);
    int appPID = atoi(aux);

    createSemaphore();
    setUpSharedMemory();

    int appIsRunning = 1;
    int k = 0;
    while (appIsRunning) {
       sem_wait(semView);
        while(*(shmAddr+k) != EOF && *(shmAddr+k) != '\0') {
            if(*(shmAddr+k) != '\0')
                putchar(*(shmAddr+k));
            k++;
        }
        int semValue;
        sem_getvalue(semView, &semValue);
        if(*(shmAddr+k) == EOF || (kill(appPID, 0) == -1 && semValue == 0))
            appIsRunning = 0;
        k++;
    }

    munmap(shmAddr, SHMSIZE);
    shm_unlink(shmName);
    endSemaphore();
    return 0;
}

void setUpSharedMemory() {
    shm_fd = shm_open(shmName, O_CREAT | O_RDONLY, 0666);
    if(shm_fd == -1) {
        printf("Can't initialize shared memory");
        exit(-1);
    }
    shmAddr = mmap(0, SHMSIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
}

void createSemaphore() {
    semView = sem_open(semViewName,O_CREAT,0644,0);
    if(semView == SEM_FAILED) {
        printf("Unable to create semaphores\n");
        sem_unlink(semViewName);
        exit(-1);
    }
}

void endSemaphore() {
    sem_close(semView);
    sem_unlink(semViewName);
}
