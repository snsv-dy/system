#include "semaphore.h"

unsigned int sems_id = 1;
int sem_id_got = 0;

int sem_init(sem_t *sem, int count){
	if(sem == NULL)
		return 1;

	if(!sem_id_got){
		syscall(SYSCALL_SEM_RAND, &sems_id);
		sem_id_got = 1;
	}

	sem->counter = count;
	sem->id = sems_id++;
	sem->name = NULL;

	return 0;
}

int sem_wait(sem_t *sem){
	if(sem == NULL){
		return 1;
	}

	syscall(SYSCALL_SEM_WAIT, sem);

	return 0;
}

int sem_post(sem_t *sem){
	if(sem == NULL){
		return 1;
	}

	syscall(SYSCALL_SEM_POST, sem);

	return 0;	
}

// nie ma alokacji pamięci więc trzeba prakazać strukturę jako parameter
int sem_open(sem_t *sem, char *name, int value){
	if(name == NULL || sem == NULL){
		return 1;
	}
	sem->name = name;
	sem->counter = value;

	int res = syscall(SYSCALL_SEM_OPEN, sem);

	return res;
}

int sem_unlink(sem_t *sem){
	if(sem == NULL){
		return 1;
	}

	int res = syscall(SYSCALL_SEM_UNLINK, sem);
	return res;
}