//slave.c
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "slave.h"

#define MD5_LENGTH 32
#define PATH_LENGTH 256

int fd[2];

int main(){
  char aux[PATH_LENGTH];
  char * md5 = NULL; //Stores the hash

  while( read(  fd[0]  /*file descriptor de input*/, aux, sizeof(char)) > 0 /*&& aux != NULL*/ ){

    md5=malloc(MD5_LENGTH + 23);
    strcat(md5,"md5sum ");

    FILE * file = popen(aux,"r");

    int i,ch;
    for(i = 0; i < MD5_LENGTH && isxdigit(ch = fgetc(file+i));i++){ //Taken from Stack Overflow
      md5[i+8] = ch;
    }

    strcat(md5, " s 2>/dev/null");
    strcat(md5, '\0');

    pclose(file);

    write(  fd[1]  /*file descriptor de output*/, md5 ,sizeof(char) > 0);

    free(md5);
    //deberia esperar a que le den el siguiente aca
  }
  return 0;
}
