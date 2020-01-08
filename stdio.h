#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include "shell.h"

int print_text(int dest, char *str);
int print_int(int dest, int num);
int print_double(int dest, double num);
int print_uhex(int dest, unsigned int num);
// int printf(int dest, char *format, ...);
int fprintf(int dest, char *format, ...);
void serial_x(unsigned int num);
int serial_d(int num);

#define PRINTF_TERM 0
#define PRINTF_SERIAL 1
// #define printf2(...) printf(PRINTF_STDOUT, __VA_ARGS__)
#define printf(...) fprintf(PRINTF_TERM, __VA_ARGS__)
#define srintf(...) fprintf(PRINTF_SERIAL, __VA_ARGS__)	// printf piszący do wyjścia szeregowego

#endif