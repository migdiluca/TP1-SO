//
//  main.c
//  Aplicacion
//
//  Created by Fermin Gomez on 8/13/18.
//  Copyright © 2018 Fermin Gomez. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SLAVES 3
#define BUFFER_SIZE 2000

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
void pipeSlaves(int * fd);
void generateSlaves();
int initialDistribution(const char * argv[], int dim);
void killSlaves();
void writeDataToBuffer(int fd, const void * buffer);
char * setUpSharedMemory(int size);
void createSemaphores();
void endSemaphores();

const char * shmName = "sharedMemoryViewAndApp";
const char * semViewName = "viewSemaphore";
const char * semAppName = "appSemaphore";

sem_t * semView;
sem_t * semApp;

int fdHash[2*SLAVES]; // hash
int fdFiles[2*SLAVES]; // archivos

char * shmAddr; // sheared memory adress (buffer)

pid_t childs[SLAVES];

int main(int argc, const char * argv[]) {
    int k = 0;
    
    char aux[8];
    sprintf(aux, "%d", getpid());
    write(STDOUT_FILENO, aux, strlen(aux)+1);
    
    int filesAmount = argc - 1;
    
    if (filesAmount == 0) {
        printf("ERROR, NO FILES TO PROCESS\n");
        return 1;
    }
    
    shmAddr = setUpSharedMemory(BUFFER_SIZE);
    
    createSemaphores();
    
    pipeSlaves(fdHash);
    pipeSlaves(fdFiles);
    
    int filesTransfered = initialDistribution(argv, argc);
    
    generateSlaves();
    
    int dataReaded = 0;
    fd_set readfds;
    
    while (dataReaded < filesAmount) {
        FD_ZERO(&readfds);
        for (int i = 0; i < SLAVES; i++) {
            FD_SET(fdHash[2*i], &readfds);
        }
        if (select(fdHash[2*(SLAVES)-1]+1, &readfds, NULL, NULL, NULL) > 0) {
            for (int i = 0; i < SLAVES; i++) {
                if (FD_ISSET(fdHash[2*i], &readfds)) {
                    int bytesReaded = read(fdHash[2*i], shmAddr + k, BUFFER_SIZE);
                    dataReaded += (int)*(shmAddr+k);
                    *(shmAddr+k) = '\0';
                    k += bytesReaded;
                    *(shmAddr+k) = EOF;
                    sem_post(semView);
                    sem_wait(semApp);
                    if (filesTransfered < filesAmount) {
                        write(fdFiles[2*i+1], argv[filesTransfered+1], strlen(argv[filesTransfered+1])+1);
                        filesTransfered++;
                    }
                }
            }
        }
    }
    killSlaves();
    endSemaphores();
    return 0;
}

void generateSlaves() {
    char * args[]= {NULL};
    for (int i = 0; i < SLAVES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fdFiles[2*i], STDIN_FILENO);
            dup2(fdHash[2*i+1], STDOUT_FILENO);
            close(fdHash[2*i]); // lectura
            close(fdFiles[2*i+1]); // escritura
            execv("./Esclavo", args); // llamada al proceso esclavo
            exit(0);
        } else {
            childs[i] = pid;
            close(fdFiles[2*i]); // lectura
            close(fdHash[2*i+1]); // escritura
        }
    }
}

void pipeSlaves(int fd[2*SLAVES]) {
    for (int i = 0; i < SLAVES; i++) {
        pipe(&fd[2*i]);
    }
}

void killSlaves() {
    for (int i = 0; i < SLAVES; i++) {
        kill(childs[i], SIGKILL);
    }
}

int initialDistribution(const char * argv[], int dim) {
    int i, j;
    // le damos un tranajo a cada esclavo
    for (i = 1, j = 0; j < SLAVES && i < dim; i++, j++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
    }
    int distribution = (dim-1)*(0.4); // completamos hasta el 40%
    j = j % SLAVES;
    for ( ; i < distribution + 1; i++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
        j = (j + 1) % SLAVES;
    }
    return --i;
}

char * setUpSharedMemory(int size) {
    int shm_fd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    char * shmAddr;
    if(shm_fd == -1 || ftruncate(shm_fd, size) == -1) {//trunca el archivo al tamaño SHMSIZE
        printf("Can't initialize shared memory");
        exit(-1);
    }
    shmAddr = mmap(0, size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    return shmAddr;
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
