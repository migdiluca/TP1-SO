//
//  main.c
//  Esclavo
//
//  Created by Fermin Gomez on 8/13/18.
//  Copyright © 2018 Fermin Gomez. All rights reserved.
//

//#include <stdio.h>
//#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/uio.h>
//#include <unistd.h>
//#define BUFFER_SIZE 20
//char buff[BUFFER_SIZE];
//
//
//int main(int argc, const char * argv[]) {
//    while (1) {
//        if (read(STDIN_FILENO, buff, BUFFER_SIZE) > 0) {
//            write(STDOUT_FILENO, buff, BUFFER_SIZE);
//        }
//    }
//    return 0;
//}

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
#define BUFFER_LENGTH 1024
char buffer[BUFFER_LENGTH];

int nextInBuffer(char * src, char * ans);

int main(){
    
    char * md5 = NULL;
    
    while(1) {
        int bytesRead = read(STDIN_FILENO, buffer, BUFFER_LENGTH);
        if (bytesRead > 0) {
            char * ret = malloc(BUFFER_LENGTH);
            int amount = 0;
            int i = 0;
            while(i < bytesRead) {
                char * path = malloc(bytesRead - i);
                i += nextInBuffer(buffer+i, path);
                amount++;
                strcat(ret+1, "<");
                strcat(ret+1, path);
                strcat(ret+1, ">: <");
                char aux[8 + BUFFER_LENGTH] = "md5sum ";
                strcat(aux, path);
                md5 = malloc(35);
                FILE * file = popen(aux,"r");
                int j,ch;
                for(j = 0; j < MD5_LENGTH && isxdigit(ch = fgetc(file));j++){   //Taken from Stack Overflow
                    md5[j] = ch;
                }
                md5[32] = '>';
                md5[33] = '\n';
                md5[34] = '\0';
                strcat(ret+1, md5);
                pclose(file);
                free(md5);
            }
            ret[0] = amount;
            write( STDOUT_FILENO, ret, strlen(ret));
            free(ret);
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

