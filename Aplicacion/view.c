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

#define SHMSIZE 1024

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
void * shmAddr;

void endSemaphores();
pid_t getApplicationPID();
void setUpSharedMemory();
void createSemaphores();

int main() {
    createSemaphores();
    setUpSharedMemory();
    pid_t appPID = getApplicationPID();

    int appIsRunning = 1;
    char buffer[100];
    while (appIsRunning) {
        sem_wait(semView);

        while(*(char*)shmAddr != EOF) {
            if(*(char*) shmAddr == '\0')
                putchar('\n');
            putchar(*(char*)shmAddr);
            shmAddr++;
        }
        sem_post(semApp);

        //chequea si el proceso existe, no lo mata
        if(kill(appPID, 0) == -1)
            appIsRunning = 0;
    }
    endSemaphores();
    return 0;
}

// REVISAR
pid_t getApplicationPID() {
    char * buff;
    int multiplier = 1;
    pid_t appPID = 0;

    read(STDIN_FILENO, buff, 1);
    while(*buff != '\0') {
        appPID = (appPID * multiplier) + (*buff - '0');
        multiplier *= 10;
        read(STDIN_FILENO, buff, 1);
    }
    return appPID;
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
