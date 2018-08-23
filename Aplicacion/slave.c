#include "slave.h"

int main(){

    char * md5 = NULL; //Stores the hash

    while(1) {
        int bytesRead = read(STDIN_FILENO, buffer, BUFFER_LENGTH);
        if (bytesRead > 0) {
            char * ret = calloc(BUFFER_LENGTH, sizeof(char));
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
                for(j = 0; j < MD5_LENGTH && isxdigit(ch = fgetc(file));j++){
                    md5[j] = ch;
                }
                md5[32] = '>';
                md5[33] = '\n';
                md5[34] = '\0';
                strcat(ret+1, md5);
                pclose(file);
                free(md5);
                free(path);
            }
            ret[0] = amount;
            write( STDOUT_FILENO, ret, strlen(ret));
            free(ret);
        }
    }
    return 0;
}

int lastBarPosition (char * string) {
    int aux = 0;
    for(int i = 0; string[i] != '\0'; i++) {
        if(string[i] == '/')
            aux = i;
    }
    return aux;
}

int nextInBuffer(char * src, char * ans) {
    int k = 0;
    int i = 0;
    int j = lastBarPosition(src);
    if(j == 0) {
        *(ans) = '"';
        k++;
    }
    while (*(src+i) != '\0') {
        *(ans+k) = *(src+i);
        i++;
        k++;
        if(i -1 == j && j!= 0) {
            *(ans+k) = '"';
            k++;
        }
    }
    *(ans+k) = '"';
    *(ans+k+1) = '\0';
    return ++i;
}
