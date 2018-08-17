//slave.c
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define MD5_LENGTH 32
#define PATH_LENGTH 256


int main(){

  char aux[PATH_LENGTH];
  char * md5 = NULL; //Stores the hash

  int sret;
  fd_set readfds;
  struct timeval timeout;


  while( 1 ){
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    sret = select(8,&readfds,NULL,NULL,&timeout);

    if(sret == 0){

      printf("Theres a slave without any tasks");

    }else{

      if(read( STDIN_FILENO, aux, sizeof(char)) > 0 /*&& aux != NULL*/){

        md5=malloc(MD5_LENGTH + 8);
        strcat(md5,"md5sum ");

        FILE * file = popen(aux,"r");

        int i,ch;

        for(i = 0; i < MD5_LENGTH && isxdigit(ch = fgetc(file+i));i++){ //Taken from Stack Overflow
          md5[i+8] = ch;
        }

        md5[MD5_LENGTH+7]='\0';

        pclose(file);

        write( STDOUT_FILENO, md5, sizeof(char) > 0);

        free(md5);

      }else{
        break ;
      }
      //deberia esperar a que le den el siguiente aca
    }
    }

  return 0;
}
