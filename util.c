#include "util.h"

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