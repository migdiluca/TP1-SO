

#ifndef SLAVE_H
#define SLAVE_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define MD5_LENGTH 32
#define BUFFER_LENGTH 1024
char buffer[BUFFER_LENGTH];

int nextInBuffer(char * src, char * ans);
int lastBarPosition (char * string);


#endif
