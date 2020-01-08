#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>

#define CEIL(a, b) (((a) + (b) - 1) / (b))

int memcpy_t(const void *src, void *dest, unsigned int len);
void memset(void *mem, char c, unsigned int size);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, int n);
int strlen(const char *str);
int strcpy(const char *src, char *dest);
int atoi_err(const char *str, int *error);
#define atoi(str) atoi_err(str, NULL)
int isspace(const char c);
int isdigit(const char c);
char *strpbrk(const char *str, const char *brk);

#define SYSCALL_OUTS 0
#define SYSCALL_GETS 1
#define SYSCALL_SWITCH_TASK 2
#define SYSCALL_SLEEP 3
#define SYSCALL_CREATE_THREAD 4
#define SYSCALL_EXIT 5
#define SYSCALL_SEM_WAIT 6
#define SYSCALL_SEM_POST 7
#define SYSCALL_START_PROCESS 8
#define SYSCALL_WAIT_FOR_FINISH 9
#define SYSCALL_SEM_OPEN 10
#define SYSCALL_SEM_UNLINK 11
#define SYSCALL_SEM_RAND 12

extern int syscall_caller(unsigned int call_type, void *data);
#define sem_wait(x) syscall_caller(SYSCALL_SEM_WAIT, x)
#define sem_post(x) syscall_caller(SYSCALL_SEM_POST, x)

#define SLEEP_SHORTEST 10
#define msleep(x) syscall_caller(SYSCALL_SLEEP, CEIL(x, SLEEP_SHORTEST))

#define RAND_MAX 32767
int rand();
void srand(unsigned int seed);

#endif