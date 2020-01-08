#include "stdlib.h"

void sleep(int duration){
	// duration = 
	syscall(SYSCALL_SLEEP, duration);
}

void yeld(){
	syscall(SYSCALL_SWITCH_TASK, 0);
}

void exit(int return_value){
	syscall(SYSCALL_EXIT, return_value);
}

int memcpy_t(const void *src, void *dest, unsigned int len){
	int i = 0;
	if(src != NULL && dest != NULL){
		for(; i < len; i++)
			*((char *)(dest + i)) = *((char *)(src + i));
	}
	return i;
}

void memset(void *mem, char c, unsigned int size){
	if(mem != NULL){
		char *t = (char *)mem;
		for(int i = 0; i < size; i++, t++)
			*t = c;
	}
}

int strcmp(const char *a, const char *b){
	if(a == NULL && b == NULL)
		return 0;

	if(a == NULL)
		return -*b;
	else if(b == NULL)
		return *a;

	int i = 0;
	for(; a[i] != '\0' && a[i] == b[i]; i++);

	return a[i] - b[i];
}

int strlen(const char *str){
	if(str == NULL)
		return 0;

	int len = 0;
	while(str[len] != '\0') len++;

	return len;
}


int strcpy(const char *src, char *dest){
	if(src == NULL || dest == NULL)
		return 1;

	int i = 0;
	for(; src[i] != '\0'; i++){
		dest[i] = src[i];
	}
	dest[i] = '\0';

	return 0;
}