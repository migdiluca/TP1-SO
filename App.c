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

#define SLAVES 3
#define BUFFER_SIZE 2000

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
void pipeSlaves(int * fd);
void generateSlaves();
int initialDistribution(const char * argv[], int dim);
void killSlaves();
void writeDataToBuffer(int fd, const void * buffer);

const  char * semName = "semaforo";
sem_t * sem;
int fdHash[2*SLAVES]; // hash
int fdFiles[2*SLAVES]; // archivos
char * shmAddr; // sheared memory adress (buffer)
pid_t childs[SLAVES];

int main(int argc, const char * argv[]) {
    int k = 0;
    printf("%d\n", getpid());
    int filesAmount = argc - 1;
    if (filesAmount == 0) {
        printf("ERROR, NO FILES TO PROCESS\n");
    }
    
    char * buff = malloc(BUFFER_SIZE);
    
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
                    int bytesReaded = read(fdHash[2*i], buff + k, BUFFER_SIZE - k);
                    if (bytesReaded > 0) {
                        dataReaded += (int)*(buff+k);
                        *(buff+k) = '\0';
                        k += bytesReaded;
                    } if (filesTransfered < filesAmount) {
                        write(fdFiles[2*i+1], argv[filesTransfered+1], strlen(argv[filesTransfered+1])+1);
                        filesTransfered++;
                    }
                }
            }
        }
    }

    killSlaves();
    free(buff);
    return 0;
}

void generateSlaves() {
    char * args[]= {NULL};
    for (int i = 0; i < SLAVES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fdFiles[2*i], STDIN_FILENO);
            dup2(fdHash[2*i+1], STDOUT_FILENO);
            close(fdHash[2*i]);
            close(fdFiles[2*i+1]);
            execv("./Esclavo", args);
            exit(0);
        } else {
            childs[i] = pid;
            close(fdFiles[2*i]);
            close(fdHash[2*i+1]);
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
    for (i = 1, j = 0; j < SLAVES && i < dim; i++, j++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
    }
    int distribution = (dim-1)*(0.4);
    j = j % SLAVES;
    for ( ; i < distribution + 1; i++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
        j = (j + 1) % SLAVES;
    }
    return --i;
}
