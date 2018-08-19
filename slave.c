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
#define BUFFER_LENGTH 100
char buffer[BUFFER_LENGTH];

int nextInBuffer(char * src, char * ans);

int main(){

  char * md5 = NULL; //Stores the hash

  while( 1 ){
    int bytesRead = read( STDIN_FILENO, buffer, BUFFER_LENGTH);
    int i = 0;

    while(i < bytesRead){
      char * path = malloc(bytesRead - i);
      i += nextInBuffer(buffer+i, path);

      char aux[8 + BUFFER_LENGTH] = "md5sum ";
      strcat(aux,path);
      md5=malloc(MD5_LENGTH);

      FILE * file = popen(aux,"r");
      int j,ch;
      for(j = 0; j < MD5_LENGTH && isxdigit(ch = fgetc(file));j++){ //Taken from Stack Overflow
        md5[j] = ch;
      }
      strcat(md5, "\0");
      pclose(file);
      write( STDOUT_FILENO, md5, BUFFER_LENGTH);
      free(md5);
    }
  }

  return 0;
}

int nextInBuffer(char * src, char * ans) {
    int i = 0;
    while (*(src+i) != '\0') {
        *(ans+i) = *(src+i);
        i++;
    }
    *(ans+i) = '\0';
    return ++i;
}
