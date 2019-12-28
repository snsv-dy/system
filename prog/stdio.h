#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include <stddef.h>
#include "syscall.h"

int printf(char *format, ...);
int gets(char *buff, int size);
char getc();
int my_scanf(char *format, ...);

int isspace(const char c);
int isdigit(const char c);

#endif