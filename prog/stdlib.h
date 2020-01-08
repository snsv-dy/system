#ifndef _STDLIB_H_
#define _STDLIB_H_

#include "syscall.h"
#include <stddef.h>

#define SLEEP_SHORTEST 10
#define CEIL(a, b) (((a) + (b) - 1) / (b))
void sleep(int duration);
#define msleep(x) sleep(CEIL(x, SLEEP_SHORTEST))

void yeld();
void exit(int return_value);
int strcmp(const char *a, const char *b);
int memcpy_t(const void *src, void *dest, unsigned int len);
void memset(void *mem, char c, unsigned int size);
int strlen(const char *str);
int strcpy(const char *src, char *dest);

#endif