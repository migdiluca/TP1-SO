//
//  main.c
//  Aplicacion
//
//  Created by Fermin Gomez on 8/13/18.
//  Copyright © 2018 Fermin Gomez. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define DEFAULTSLAVES 5
#define BUFFER_SIZE 100

#define SHMSIZE 1024

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
void pipeSlaves(int * fd);
void generateSlaves();
void killSlaves();
void writeDataToBuffer(int fd, const void * buffer);
void endSemaphores();
void createSemaphores();
void setUpSharedMemory();
void shareMyPID();
int getNumberOfCores();
void initializeArrays();
void freeArrays();

const char * shmName = "viewAndAppSharedMemory";
const char * semViewName = "viewSemaphore";
const char * semAppName = "appSemaphore";

sem_t * semView;
sem_t * semApp;
int shm_fd; // shared memory address (buffer)
void * shmAddr; //shared memory pointer
int * fdHash; // hash
int * fdFiles; // archivos
pid_t * childs;
int numOfSlaves;

int main(int argc, const char * argv[]) {
    int dim = 0;
    printf("%d", getpid()); // manda a salida estandard el pid del processo para que sea leido por la vista

    int filesAmount = argc - 1;
    
    if (filesAmount == 0) {
        printf("ERROR, NO FILES TO PROCESS\n");
        return 0;
    }

    numOfSlaves = getNumberOfCores();
    initializeArrays();
    
    pipeSlaves(fdHash);
    pipeSlaves(fdFiles);

    generateSlaves();
    createSemaphores();
    setUpSharedMemory();
    shareMyPID();

    int initialDistribution = numOfSlaves; // filesAmount*0.4
    int j = 0;
    for (int i = 1; i < initialDistribution; i++) {
        if (write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1) == -1) { // +1 para que ponga el null
            printf("Error: %s\n", strerror(errno));
            return 1;
        }
        j = (j + 1) % numOfSlaves;
    }
    
    int filesTransfered = initialDistribution;
    int dataReaded = 0;
    fd_set readfds;
    
    while (dataReaded < filesAmount) {
        FD_ZERO(&readfds);
        for (int i = 0; i < numOfSlaves; i++) {
            FD_SET(fdHash[2*i], &readfds);
        }
        if (select(fdHash[2*(numOfSlaves-1)]+1, &readfds, NULL, NULL, NULL) > 0) { // hay informacion disponible en algun fd
            for (int i = 0; i < numOfSlaves; i++) {
                if (FD_ISSET(fdHash[2*i], &readfds)) {
                    sem_post(semView); //hay que ver esto
                    int bytesReaded = read(fdHash[2*i], shmAddr + dim, BUFFER_SIZE - dim);
                    sem_wait(semApp); //hay que ver esto
                    if (bytesReaded == -1) {
                        printf("Error: %s\n", strerror(errno));
                        return 1;
                    }
                    if (bytesReaded > 0) {
                        dataReaded++;
                        dim += dataReaded;
                    } if (filesTransfered < filesAmount) {
                        if (write(fdFiles[2*i+1], argv[filesTransfered+1], strlen(argv[filesTransfered+1])+1) == -1) {
                            printf("Error: %s\n", strerror(errno));
                            return 1;
                        }
                        filesTransfered++;
                    }

                }
            }
        }
    }

    killSlaves();
    //cerramos el semaforo y lo borramos
    endSemaphores();
    freeArrays();
    return 0;
}

void shareMyPID(){
    char pid[21];
    int myPID = (int) getpid();
    int lenght = sprintf(pid, "%d", myPID);
    pid[lenght] = '\0';
    write(STDOUT_FILENO, pid, lenght + 1);
}

void setUpSharedMemory() {
    shm_fd = shm_open(shmName, O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1 || ftruncate(shm_fd, SHMSIZE) == -1) {//trunca el archivo al tamaño SHMSIZE
        printf("Can't initialize shared memory");
        exit(-1);
    }
    shmAddr = mmap(0, SHMSIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

void generateSlaves() {
    char * args[]= {};
    for (int i = 0; i < numOfSlaves; i++) {
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

void pipeSlaves(int fd[]) {
    for (int i = 0; i < numOfSlaves; i++) {
        pipe(&fd[2*i]);
    }
}

void killSlaves() {
    for (int i = 0; i < numOfSlaves; i++) {
        kill(childs[i], SIGKILL);
    }
}

//Retorna numero de nucleos, o si no puede un default.
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


//void writeDataToBuffer(int fd, const void * buffer) {
//    int fdSize = sizeof(fd); // que onda con esto??
//    int bytesWritten = 0;
//    while (bytesWritten < fdSize) {
//        int bytesSend = write(fd, buffer, (fdSize-bytesWritten));
//        if (bytesSend > 0) {
//            bytesWritten += bytesSend;
//        }
//    }
//}

////struct sigaction {
////    void       (*sa_handler)(int);
////    sigset_t   sa_mask;
////    int        sa_flags;
////    void       (*sa_sigaction)(int, siginfo_t *, void *);
////};
//
////void actionHandler (int signum) {
////    switch (signum) {
////        case SIGUSR1:
////            sem_wait(sem);
////            writeDataToBuffer();
////            sem_post(sem);
////            break;
////        default:
////            break;
////    }
////
////}
//
//void actionHandler(int signum, siginfo_t * info, void * context) {
//    switch (signum) {
//        case SIGUSR1: // senal de que el escalvo termino de realizar su tarea
//            sem_wait(sem);
//            writeDataToBuffer();
//            // mandamos un archivo a procesar al esclavo con info->si_pid (pid del escalvo que finalizo)
//            sem_post(sem);
//            break;
//        default:
//            break;
//    }
//}
//
//// set up de senales
//struct sigaction action;
////action.sa_handler = &actionHandler;
//sigemptyset(&action.sa_mask);
////action.sa_flags = 0;
//action.sa_flags = SA_SIGINFO;
//action.sa_sigaction = &actionHandler;
//sigaction(SIGUSR1, &action, NULL); // mapeo senal de finalizacion de tarea de hash
