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
#include <sys/shm.h>
#include <assert.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <string.h>
#define SLAVES 5
#define BUFFER_SIZE 1

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
//void actionHandler (int signum);
void actionHandler(int signo, siginfo_t *info, void *context);
void writeDataToBuffer();
void pipeSlaves(int * fd);


const  char * semName = "semaforo";
sem_t * sem;
int fdHash[2*SLAVES]; // hash
int fdFiles[2*SLAVES]; // archivos
char * shmAddr; // sheared memory adress (buffer)
pid_t childs[SLAVES];

int main(int argc, const char * argv[]) {

    int filesAmount = argc - 1;

    if (filesAmount == 0) {
        // error no hay archivos
    }
    
    pipeSlaves(fdHash);
    pipeSlaves(fdFiles);

    generateSlaves();

    // creamos memoria compratida
    int shmId = allocateSharedMemory(BUFFER_SIZE);
    if (shmId == -1) {
        perror("shmget");
        exit(1);
    }

    // mapeamos memoria compartida
    shmAddr = (char *) mapSharedMemory(shmId);
    if (!shmAddr) {
        perror("shmat");
        exit(1);
    }
    
    // el semaforo se inicializa en 0
    sem = sem_open(semName, O_CREAT|O_EXCL , S_IRUSR| S_IWUSR , 0);
    
    // set up de senales
    struct sigaction action;
    //action.sa_handler = &actionHandler;
    sigemptyset(&action.sa_mask);
    //action.sa_flags = 0;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &actionHandler;
    sigaction(SIGUSR1, &action, NULL); // mapeo senal de finalizacion de tarea de hash
    
    int initialDistribution = filesAmount*0.4;
    int j = 0;
    for (int i = 1; i < initialDistribution; i++) {
        write(fdHash[2*j], argv[i], strlen(argv[i])+1); // +1 para que ponga el null
        j = (j + 1) % SLAVES;
    }
    int filesTransfered = initialDistribution;
    fd_set readfds;
    while (filesTransfered < filesAmount) { // modificar para que pare cuando se queda sin archivos para mandar a los esclavos
        FD_ZERO(&readfds);
        for (int i = 0; i < SLAVES; i++) {
            FD_SET(fdHash[2*i], &readfds);
        }
        if (select(fdHash[2*(SLAVES-1)+1], &readfds, NULL, NULL, NULL) > 0) { // hay informacion disponible en algun fd
            for (int i = 0; i < SLAVES; i++) {
                if (FD_ISSET(fdHash[2*i], &readfds)) {
                    // hay informacion en el fd la leo y se la paso a la vista >> IMPLEMENTAR
                    // le pasamos un archivo mas al esclavo
                    write(fdHash[2*i], argv[filesTransfered], strlen(argv[filesTransfered])+1); // +1 para que ponga el null
                    filesTransfered++;
                }
            }
        }
    }
    
    // cerramos el semaforo
    sem_close(sem);
    return 0;
}

void * mapSharedMemory(int id) {
    void * addr;
    assert(id != 0);
    addr = shmat(id, NULL, 0);
    // shmctl(id, IPC_RMID, NULL); NOSE QUE ES ESTO
    return addr;
}

int allocateSharedMemory(int n) {
    assert(n > 0);
    return shmget(IPC_PRIVATE, n, IPC_CREAT | SHM_R | SHM_W); // averiguar que es IPC_PRIVATE
}

//void actionHandler (int signum) {
//    switch (signum) {
//        case SIGUSR1:
//            sem_wait(sem);
//            writeDataToBuffer();
//            sem_post(sem);
//            break;
//        default:
//            break;
//    }
//
//}

void actionHandler(int signum, siginfo_t * info, void * context) {
    switch (signum) {
        case SIGUSR1: // senal de que el escalvo termino de realizar su tarea
            sem_wait(sem);
            writeDataToBuffer();
            // mandamos un archivo a procesar al esclavo con info->si_pid (pid del escalvo que finalizo)
            sem_post(sem);
            break;
        default:
            break;
    }
}

void writeDataToBuffer() {

}

void generateSlaves() {
    for (int i = 0; i < SLAVES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // cierro extremos del pipe que el hijo no va a utilizar
            dup2(fdFiles[2*i],STDIN_FILENO);
            dup2(fdHash[2*i+1],STDOUT_FILENO);
            close(fdFiles[2*i]); // lectura
            close(fdHash[2*i]); // lectura
            close(fdFiles[2*i+1]); // escritura
            close(fdHash[2*i+1]); // escritura
            char * args[]= {NULL};
            execv("./Esclavo", args); // llamada al proceso esclavo
        } else {
            childs[i] = pid;
            // cierro extremos del pipe que el padere no va a utilizar
            close(fdFiles[2*i]); // lectura
            close(fdHash[2*i]); // lectura
            close(fdFiles[2*i+1]); // escritura
            close(fdHash[2*i+1]); // escritura
        }
    }
}

void pipeSlaves(int * fd) {
    for (int i = 0; i < SLAVES; i++) {
        pipe(&fd[2*i]);
    }
}


//struct sigaction {
//    void       (*sa_handler)(int);
//    sigset_t   sa_mask;
//    int        sa_flags;
//    void       (*sa_sigaction)(int, siginfo_t *, void *);
//};
