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

#define SLAVES 5
#define BUFFER_SIZE 1

void * mapSharedMemory(int id);
int allocateSharedMemory(int n);
//void actionHandler (int signum);
void actionHandler(int signo, siginfo_t *info, void *context);
void writeDataToBuffer();


const  char * semName = "semaforo";
sem_t * sem;
int fd1[2]; // hash
int fd2[2]; // archivos
char * shmAddr; // sheared memory adress (buffer)

int main(int argc, const char * argv[]) {
    pipe(fd1);
    pipe(fd2);
    int files = argc - 1;
    if (files == 0) {
        // error no hay archivos
    }
    pid_t childs[SLAVES];
    
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

    // genero esclavos
    for (int i = 0; i < SLAVES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // HIJO I
            // cierro extremos del pipe que el hijo no va a utilizar
            close(fd2[1]);
            close(fd1[0]);
            char * args[]= {}; //argumentos para el esclavo
            execv("./Esclavo", args); // llamada al proceso esclavo
            exit(0);
        } else if (pid > 0) {
            childs[i] = pid;
        } else {
            //caso de error
        }
    }
    // PADRE
    // cierro extremos del pipe que el padere no va a utilizar
    close(fd2[0]);
    close(fd1[1]);
    
    // set up de senales
    struct sigaction action;
    //action.sa_handler = &actionHandler;
    sigemptyset(&action.sa_mask);
    //action.sa_flags = 0;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = &actionHandler;
    sigaction(SIGUSR1, &action, NULL); // mapeo senal de finalizacion de tarea de hash
    
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
        case SIGUSR2:
            
        default:
            break;
    }
}

void writeDataToBuffer() {
    size_t nbytes; // como se cuantas bytes tengo que levantar
    read(fd1[0], shmAddr, nbytes);
}


//struct sigaction {
//    void       (*sa_handler)(int);
//    sigset_t   sa_mask;
//    int        sa_flags;
//    void       (*sa_sigaction)(int, siginfo_t *, void *);
//};






