#include <stddef.h>
#include "syscall.h"
#include "stdio.h"
#include "thread.h"
#include "stdlib.h"
#include "semaphore.h"

#define ARR_LEN 0x1000


int overflower(int i, int *arr){
	if(i < 2049){
		int a[1024];
		for(int i = 0; i < 1024; i++){
			a[i] = 0;
		}
		printf("%d", i);
		return overflower(++i, a);
	}
	printf("overflow %d, end\n", i);

	return 0;
}

void *th_fn(void *a){
	// int c = 125;
	// char big_arr[ARR_LEN];
	// big_arr[0] = 'a';
	// big_arr[4095] = 'c';
	// printf("Just a normal printf, nothing unusual, arr_addr: %x, c addr: %x\n", big_arr, &c);
	// printf("Just a normal printf, nothing unusual\n");
	// c = 21;
	// printf("c: %d\n", c);
	overflower(1, NULL);

	return NULL;
}

sem_t send, recive;

void *thread1(void *a){

	while(1){
		sem_wait(&send);
		printf("sending\n");
		msleep(1000);
		sem_post(&recive);
	}

	return a;
}


void *thread2(void *a){

	while(1){
		sem_wait(&recive);
		printf("recived\n");
		sem_post(&send);
	}

	return a;
}



int main(){
	// sem_init(&send, 1);
	// sem_init(&recive, 0);
	sem_open(&send, "sender", 1);
	sem_open(&recive, "reciver", 0);

	printf("sendid: %d, recvid: %d\n", send.id, recive.id);

	thread_t th1, th2;
	printf("Creating thread\n");
	int res = create_thread(&th1, thread1, NULL);
	res = create_thread(&th2, thread2, NULL);
	printf("Thread will have pid: %d\n", res);
	if(res >= 0){
		wait(res);
	}
	printf("exiting program\n");

	// overflower(1, NULL);

	sem_unlink(&send);
	sem_unlink(&recive);

	return 0;
}