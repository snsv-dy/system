#include <stddef.h>
#include "syscall.h"
#include "stdio.h"

#define SLEEP_SHORTEST 10
#define CEIL(a, b) (((a) + (b) - 1) / (b))
void sleep(int duration){
	// duration = 
	syscall(SYSCALL_SLEEP, duration);
}
#define msleep(x) sleep(CEIL(x, SLEEP_SHORTEST))

void yeld(){
	syscall(SYSCALL_SWITCH_TASK, 0);
}

void exit(int return_value){
	syscall(SYSCALL_EXIT, return_value);
}

// struktura semafora
typedef struct{
	int counter;
	int id;
	char *name;
} sem_t;

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
void thread_func_caller(thread_t *thread, void *(*func)(void *), void *param){
	// void *res = pack->func(pack->param);
	// pack->thread->return_value = res;
	thread->return_value = func(param);

	printf("End of thread func\n");

	exit(0);

	while(1){
		printf("Still going\n");
		msleep(500);
	}

	// sygnał o tym żeby zniszczyć wątek
}

int create_thread(thread_t *thread, void *(*func)(void *), void *param){
	if(thread == NULL || func == NULL || param == NULL){
		return 1;
	}

	thread_pack pack;
	pack.thread = thread;
	pack.func = func;
	pack.param = param;
	pack.wrap_func = thread_func_caller;

	syscall(SYSCALL_CREATE_THREAD, &pack);

	return 0;
}


int create_process(int id){
	return syscall(SYSCALL_START_PROCESS, (void *)id);
}

int wait(int pid){
	return syscall(SYSCALL_WAIT_FOR_FINISH, pid);
}

void *basic_thread_func(void *a){
	int c;
	printf("Stack at childed thread: %x\n", &c);

	return NULL;
}

unsigned int sems_id = 1;

int sem_init(sem_t *sem, int count){
	if(sem == NULL)
		return 1;

	sem->counter = count;
	sem->id = sems_id++;
	sem->name = NULL;

	return 0;
}

sem_t recv_sem, send_sem;

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

char shared_buff[100];

void *thread_func1(void *a){

	printf("I am thread 1, i will be only seming\n");
	while(1){
		sem_wait(&recv_sem);
		// // printf("[thread1] Text recived: %s", shared_buff);
		// char last = shared_buff[0];
		// for(int i = 0; shared_buff[i] != '\0'; i++){
		// 	last = shared_buff[i];
		// }

		// printf("last char: %d", last);
		printf("[thread1] Text recived: ");
		printf(shared_buff);
		// printf("\n");
		sem_post(&send_sem);
		// syscall(SYSCALL_PUTS, "[thread1] Sem released\n");
		// msleep(300);
	}

	return NULL;
}

void *thread_func2(void *a){
	printf("thread2, i will be reciving input\n");
	send_sem.counter++;
	char *limp = "bizkit\n";
	int i = 0;
	for(; limp[i] != '\0'; i++){
		shared_buff[i] = limp[i];
	}
	shared_buff[i] = '\0';

	msleep(500);

	while(1){
		sem_wait(&send_sem);
		printf("[thread2] Enter some text: ");
		gets(shared_buff, 100);
		// printf("gets end\n");
		// msleep(1000);
		sem_post(&recv_sem);
	}
}

void *thread_func3(void *a){
	// printf("         I'm just here to die :(\n");

	printf("Dying with loop\n");

	for(;;);

	return NULL;
}

int main(){

	sem_init(&send_sem, 1);
	sem_init(&recv_sem, 0);

	// int k = 0;

	thread_t thread, thread2, thread3;
	int input_pid = create_thread(&thread2, thread_func2, 8);
	// create_thread(&thread, thread_func3, 8);
	create_thread(&thread, thread_func1, 8);



	// while(1){
	// 	sleep(10000);
	// }

	// thread_t thr;
	// create_thread(&thr, thread_func3, NULL);

	// printf("Dying as a whole process\n");

	// printf("Creating process 1\n");
	// int pid = create_process(1);
	// printf("Created/requested: %d\nWaiting for end\n", pid);
	// int res = wait(pid);
	// printf("Finished waiting for %d, %d\n", pid, res);
	// printf("Dying\n");

	wait(input_pid);

	exit(0);

	return 0;
}