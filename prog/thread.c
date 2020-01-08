#include "thread.h"
// void thread_func_caller(thread_pack *pack){
void thread_func_caller(thread_t *thread, void *(*func)(void *), void *param){
	// void *res = pack->func(pack->param);
	// pack->thread->return_value = res;
	thread->return_value = func(param);

	exit(0);
}

int create_thread(thread_t *thread, void *(*func)(void *), void *param){
	if(thread == NULL || func == NULL){
		return -3;
	}

	thread_pack pack;
	pack.thread = thread;
	pack.func = func;
	pack.param = param;
	pack.wrap_func = thread_func_caller;

	return syscall(SYSCALL_CREATE_THREAD, &pack);
}


int create_process(int id){
	return syscall(SYSCALL_START_PROCESS, (void *)id);
}

int wait(int pid){
	return syscall(SYSCALL_WAIT_FOR_FINISH, pid);
}