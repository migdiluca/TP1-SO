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

pid_t getApplicationPID();
void endSemaphores();
void createSemaphores();
void setUpSharedMemory();

const char * shmName = "sharedMemoryViewAndApp";
const char * semViewName = "viewSemaphore";
const char * semAppName = "appSemaphore";

sem_t * semView;
sem_t * semApp;
int shm_fd;
char * shmAddr;

void endSemaphores();
pid_t getApplicationPID();
void setUpSharedMemory();
void createSemaphores();

int main(int argc, const char * argv[]) {
    //Getting app PID
    char aux[8];
    read(STDIN_FILENO, aux, 8);

    createSemaphores();
    setUpSharedMemory();
    
    int appPID = atoi(aux);
    
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
    endSemaphores();
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

void createSemaphores() {
    semView = sem_open(semViewName,O_CREAT,0644,0);
    semApp = sem_open(semAppName,O_CREAT,0644,0);
    if(semApp == SEM_FAILED || semView == SEM_FAILED) {
        printf("Unable to create semaphores\n");
        sem_unlink(semViewName);
        sem_unlink(semAppName);
        exit(-1);
    }
}

void endSemaphores() {
    sem_close(semView);
    sem_unlink(semViewName);
    sem_close(semApp);
    sem_unlink(semAppName);
}
