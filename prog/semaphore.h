#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "syscall.h"
#include <stddef.h>
// struktura semafora
typedef struct{
	int counter;
	int id;
	char *name;
} sem_t;


void *basic_thread_func(void *a);
int sem_init(sem_t *sem, int count);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
// nie ma alokacji pamięci więc trzeba prakazać strukturę jako parameter
int sem_open(sem_t *sem, char *name, int value);
int sem_unlink(sem_t *sem);

#endif