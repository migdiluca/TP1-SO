//testing.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_LENGTH 100
#define MD5_LENGTH 32

int main(){
  FILE * testFile;
  FILE * file;
  char reader[PATH_LENGTH];
  int i,j=0;
  char c;
  int textFile = 0;

  char aux[8+PATH_LENGTH];

  int md5 = 0;
  int counter;

  testFile = fopen("md5Hashes.txt", "r");
  if(testFile == NULL){
    printf("%s\n", "ERROR: md5Hashes.txt is corrupted or doesn't exist" );
  }
  char buff[33];


  while((c = fgetc(testFile)) != EOF){
    switch(c){
      case '<':
              if(!md5){
                strcpy(aux, "md5sum ") ;
                counter = 0;
              }
              break;
      case '>' :
                  textFile = 0;
                  if(!md5){
                    aux[counter + 7] = '\0';
                    if(strcmp(aux, "md5sum ./md5Hashes.txt") == 0){
                      textFile = 1;
                      printf("%s\n", "El archivo de texto sera ignorado");
                    }else{
                      printf("Calculando %s\n", aux);
                      file = popen(aux, "r");
                      int ch;
                      for(j = 0; j < MD5_LENGTH && isxdigit(ch = fgetc(file));j++){
                        buff[j] = ch;
                      }
                      buff [j] = '\0';
                      printf("El md5 es:%s\n", buff);
                      pclose(file);
                    }
                    j=0;
                    md5 = 1;
                  }else{
                    printf("%s\n", "Coinciden");
                    md5 = 0;
                  }
                  counter = 0;
                  break;
      case ' ':   break;
      case '\n':  break;
      case ':' :  break;
      default :
              if(textFile){break;}
              if(!md5){
                aux[7 + counter] = c;
                counter++;
              }else{
                printf("%c - %c\n",buff[j], c );
                if(buff[j++] != c){
                  printf("Error con el calculo de: %s\n", aux);
                  return 1;
                }
              }
              break;
    }
  }
  fclose(testFile);
  printf("No se han encontrado problemas con los hashes de los archivos\n");
  return 0;
}
