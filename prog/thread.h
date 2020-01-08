#ifndef _THREAD_H_
#define _THREAD_H_

#include <stddef.h>
#include "syscall.h"

typedef struct{
	int pid;
	void *return_value;
} thread_t;

typedef struct thread_pck{
	thread_t *thread;
	void *(*func)(void *);
	void *param;
	void (*wrap_func)(struct thread_pck *);	// wskaźnik na funkcję pomiżej
} thread_pack; // struktura przekazywana do syscalla, przy tworzeniu wątku


// void thread_func_caller(thread_pack *pack){
void thread_func_caller(thread_t *thread, void *(*func)(void *), void *param);
int create_thread(thread_t *thread, void *(*func)(void *), void *param);
int create_process(int id);
int wait(int pid);

#endif