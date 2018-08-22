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
void createSemaphores();
void endSemaphores();
int getNumberOfCores();
void initializeArrays();
void freeArrays();

const char * shmName = "sharedMemoryViewAndApp";
const char * semViewName = "viewSemaphore";
const char * semAppName = "appSemaphore";

sem_t * semView;
sem_t * semApp;

int numOfSlaves = DEFAULTSLAVES;

int *fdHash; // hash
int *fdFiles; // archivos

char * shmAddr; // sheared memory adress (buffer)

pid_t *childs;

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
    
    numOfSlaves = getNumberOfCores();
    initializeArrays();
    shmAddr = setUpSharedMemory(SHMSIZE);
    createSemaphores();
    
    pipeSlaves(fdHash);
    pipeSlaves(fdFiles);
    
    int filesTransfered = initialDistribution(argv, argc);
    
    generateSlaves();
    
    int dataReaded = 0;
    fd_set readfds;
    while (dataReaded < filesAmount) {
        FD_ZERO(&readfds);
        for (int i = 0; i < numOfSlaves; i++) {
            FD_SET(fdHash[2*i], &readfds);
        }
        if (select(fdHash[2*(numOfSlaves)-1]+1, &readfds, NULL, NULL, NULL) > 0) {
            for (int i = 0; i < numOfSlaves; i++) {
                if (FD_ISSET(fdHash[2*i], &readfds)) {
                    int filesInThisPipe[1];
                    read(fdHash[2*i], filesInThisPipe, 1);
                    int bytesReaded = read(fdHash[2*i], shmAddr + k, SHMSIZE);
                    dataReaded += *filesInThisPipe;
                    k += bytesReaded;
                    if (filesTransfered < filesAmount) {
                        write(fdFiles[2*i+1], argv[filesTransfered+1], strlen(argv[filesTransfered+1])+1);
                        filesTransfered++;
                    }
                }
            }
            *(shmAddr+k) = '\0';
            k++;
            sem_post(semView);
        }
    }

    *(shmAddr+k) = EOF;
    munmap(shmAddr, SHMSIZE);
    shm_unlink(shmName);
    killSlaves();
    freeArrays();
    sem_post(semView);
    endSemaphores();
    return 0;
}

void generateSlaves() {
    char * args[]= {NULL};
    for (int i = 0; i < numOfSlaves; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fdFiles[2*i], STDIN_FILENO);
            dup2(fdHash[2*i+1], STDOUT_FILENO);
            close(fdHash[2*i]); // lectura
            close(fdFiles[2*i+1]); // escritura
            struct stat aux;
            if(stat ("./Slave", &aux) != 0){
                sprintf(shmAddr, "./Slave does not exist. Exiting...");
                sem_post(semView);
                exit(-1);
            }
            execv("./Slave", args); // llamada al proceso esclavo
            exit(0);
        } else {
            childs[i] = pid;
            close(fdFiles[2*i]); // lectura
            close(fdHash[2*i+1]); // escritura
        }
    }
}

void pipeSlaves(int fd[2*numOfSlaves]) {
    for (int i = 0; i < numOfSlaves; i++) {
        pipe(&fd[2*i]);
    }
}

void killSlaves() {
    for (int i = 0; i < numOfSlaves; i++) {
        kill(childs[i], SIGKILL);
    }
}

int initialDistribution(const char * argv[], int dim) {
    int i, j;
    // le damos un tranajo a cada esclavo
    for (i = 1, j = 0; j < numOfSlaves && i < dim; i++, j++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
    }
    int distribution = (dim-1)*(0.4); // completamos hasta el 40%
    j = j % numOfSlaves;
    for ( ; i < distribution + 1; i++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
        j = (j + 1) % numOfSlaves;
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

int getNumberOfCores() {
#if defined(__linux__)
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return DEFAULTSLAVES;
}

void initializeArrays() {
    fdHash = malloc(sizeof(int) * 2 * numOfSlaves);
    fdFiles = malloc(sizeof(int) * 2 * numOfSlaves);
    childs = malloc(sizeof(pid_t) * numOfSlaves);
}

void freeArrays() {
    free(fdHash);
    free(fdFiles);
    free(childs);
}
