#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include "shell.h"

int print_text(char *str);
int print_int(int num);
int print_double(double num);
int print_uhex(unsigned int num);
// int printf(int dest, char *format, ...);
int printf(char *format, ...);
void serial_x(unsigned int num);
int serial_d(int num);

#define PRINTF_STDOUT 0
#define PRINTF_SERIAL 1
// #define printf2(...) printf(PRINTF_STDOUT, __VA_ARGS__)

 
#endif