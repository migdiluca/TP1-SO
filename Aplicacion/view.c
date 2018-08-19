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

#define SHMSIZE 1024 //DESPUES VEMOS EL TAMAÑO

#define STDI

const char * shmName = "viewAndAppSharedMemory";
const char * semName = "viewAndAppSemaphore";

int shm_fd;
void * shmAddr;
sem_t *sem;

void endSemaphore();
pid_t getApplicationPID();
void setUpSharedMemory();
void createSemaphore();

int main() {
    createSemaphore();
    setUpSharedMemory();

    pid_t appPID = getApplicationPID();

    int appIsRunning = 1;
    char buffer[100];
    while (appIsRunning) {
        sem_wait(sem);
        /*Hay que leer el buffer, por ahora supongo que me lo mandan armado y cuando no
        hay nada mas hay un EOF, despues sigo buscando como es esto */
        while(*(char*)shmAddr != EOF) {
            if(*(char*) shmAddr == '\0')
                printf("\n");
            printf("%c", *(char*)shmAddr);
            shmAddr++;
        }
        sem_post(sem);

        //chequea si el proceso existe, no lo mata
        if(kill(appPID, 0) == -1)
            appIsRunning = 0;
    }
    shm_unlink(semName);
    endSemaphore();
    return 0;
}

// REVISAR
pid_t getApplicationPID() {
    char buff;
    int multiplier = 1;
    pid_t appPID = 0;

    read(STDIN_FILENO, &buff, 1);
    while(buff != '\0') {
        appPID = (appPID * multiplier) + (buff - '0');
        multiplier *= 10;
        read(STDIN_FILENO, &buff, 1);
    }
    return appPID;
}

void setUpSharedMemory() {
    shm_fd = shm_open(shmName, O_RDONLY, 0666);
    if(shm_fd == -1) {
        printf("Can't initialize shared memory");
        exit(-1);
    }
    shmAddr = mmap(0, SHMSIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
}

void createSemaphore() {
    sem = sem_open(semName,O_CREAT,0644,1);
    if(sem == SEM_FAILED) {
        printf("Unable to create semaphore\n");
        sem_unlink(semName);
        exit(-1);
    }
}

void endSemaphore() {
    sem_close(sem);
    sem_unlink(semName);
}
