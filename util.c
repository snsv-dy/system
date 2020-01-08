#include "util.h"

long int next_rand = 1;

int rand(){
	next_rand = next_rand * 1103515245 + 12345;
	return (unsigned int)(next_rand / (RAND_MAX * 2) % RAND_MAX);
}

void srand(unsigned int seed){
	next_rand = seed;
}

char *strpbrk(const char *str, const char *brk){
	if(str == NULL || brk == NULL){
		return NULL;
	}

	for(int i = 0; str[i] != '\0'; i++){
		for(int j = 0; brk[j] != '\0'; j++){
			if(str[i] == brk[j])
				return str + i;
		}
	}

	return NULL;
}

int isdigit(const char c){
	return c >= '0' && c <= '9';
}

int isspace(const char c){
	return c == ' ' || c =='\t' || c == '\n';
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

int strncmp(const char *a, const char *b, int n){
	if(a == NULL && b == NULL)
		return 0;

	if(a == NULL)
		return -*b;
	else if(b == NULL)
		return *a;

	int i = 0;
	for(; i < n && a[i] != '\0' && a[i] == b[i]; i++);
	
	if(i == n) i--;

	return a[i] - b[i];
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

extern fprintf(int dest, char *format, ...);

int atoi_err(const char *str, int *error){
	if(str == NULL){
		if(error != NULL)
			*error = 1;

        return 0;
	}
    
    char *ptr = str;
    int numlen = 0;
    for(; isspace(ptr[0]) && ptr[0] != '\0'; ptr++);
        
    int minus = 1;
    if(ptr[0] == '-'){
        minus = -1;
        ptr++;
    }

    int res = 0;

    // while((isdigit(ptr[0]) || (isspace(ptr[0]) && numlen == 0)) && numlen < 10){
    while(isdigit(ptr[0]) && numlen < 10){
        if(numlen > 0) res *= 10;
        
        res += ptr[0] - '0';
        numlen++;
        ptr++;
    }
    
    res *= minus;

    // fprintf(0, "numlen: %d\n", numlen);

	if(numlen == 0 && error != NULL){
		*error = 2;
		return res;
	}

	if(error != NULL)
		*error = 0;
    
    return res;
}