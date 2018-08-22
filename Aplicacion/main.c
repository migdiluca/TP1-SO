#include "main.h"
#include "shmSem.h"
int main(int argc, const char * argv[]) {
    sem = createSemaphore(semName);
    shmAddr = setUpSharedMemory(SHMSIZE, shmName);
    char aux[8];
    sprintf(aux, "%d", getpid());
    write(STDOUT_FILENO, aux, strlen(aux)+1);
    int filesAmount = argc - 1;
    if (filesAmount == 0) {
        printf("ERROR, NO FILES TO PROCESS\n");
        sem_post(sem);
        return 1;
    }
    
    FILE* outFile;
    outFile = fopen("md5Hashes.txt","w");
    if(outFile==NULL) {
        perror("File couldn't be created");
        sem_post(sem);
        exit(1);
    }
    
    numOfSlaves = getNumberOfCores();
    initializeArrays();
    
    pipeSlaves(fdHash);
    pipeSlaves(fdFiles);
    
    int filesTransfered = initialDistribution(argv, argc);
    
    generateSlaves();
    
    fd_set readfds;
    
    int dataReaded = 0;
    int k = 0;
    int chunk = 0;
    while (dataReaded < filesAmount) {
        FD_ZERO(&readfds);
        for (int i = 0; i < numOfSlaves; i++) {
            FD_SET(fdHash[2*i], &readfds);
        }
        int numPending = select(fdHash[2*(numOfSlaves)-1]+1, &readfds, NULL, NULL, NULL);
        if ( numPending > 0) {
            for (int i = 0; i < numOfSlaves && numPending > 0; i++) {
                if (FD_ISSET(fdHash[2*i], &readfds)) {
                    numPending--;
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
            fprintf(outFile, "%s", shmAddr+chunk);
            *(shmAddr+k) = '\0';
            k++;
            chunk = k;
            sem_post(sem);
        }
    }
    
    *(shmAddr+k) = EOF;
    munmap(shmAddr, SHMSIZE);
    shm_unlink(shmName);
    killSlaves();
    freeArrays();
    fclose(outFile);
    sem_post(sem);
    endSemaphore(sem, semName);
    return 0;
}

void generateSlaves() {
    for (int i = 0; i < numOfSlaves; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fdFiles[2*i], STDIN_FILENO);
            dup2(fdHash[2*i+1], STDOUT_FILENO);
            close(fdHash[2*i]);
            close(fdFiles[2*i+1]);
            struct stat aux;
            if(stat ("./Slave", &aux) != 0){
                sprintf(shmAddr, "./Slave does not exist. Exiting...\n ");
                sem_post(sem);
                exit(-1);
            }
            char * args[]= {NULL};
            execv("./Slave", args);
            exit(0);
        } else {
            childs[i] = pid;
            close(fdFiles[2*i]);
            close(fdHash[2*i+1]);
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
    for (i = 1, j = 0; j < numOfSlaves && i < dim; i++, j++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
    }
    int distribution = (dim-1)*(0.4);
    j = j % numOfSlaves;
    for ( ; i < distribution + 1; i++) {
        write(fdFiles[2*j+1], argv[i], strlen(argv[i])+1);
        j = (j + 1) % numOfSlaves;
    }
    return --i;
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
