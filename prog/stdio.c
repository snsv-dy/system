#include "stdio.h"


int isdigit(const char c){
	return c >= '0' && c <= '9';
}

int isspace(const char c){
	return c == ' ' || c =='\t' || c == '\n';
}


#define BUFFER_SIZE 10000
char in_buffer[BUFFER_SIZE];
int in_buffer_length;
int in_buffer_pos;
int in_set = 0;

char out_buffer[BUFFER_SIZE];
int out_buffer_length;
int out_set = 0;


#define is_buffer_full() (out_buffer_length == (BUFFER_SIZE - 1))

#define clear_out_buffer() {\
	out_buffer[0] = '\0'; \
	out_buffer_length = 0; \
}

#define out_flush() { \
	syscall(SYSCALL_PUTS, out_buffer); \
	clear_out_buffer(); \
}

#define buffer_full_routine() { \
	if(is_buffer_full()){ \
		out_flush(); \
	} \
}
#define out_buffer_putc(c) { \
	out_buffer[out_buffer_length] = c; \
	out_buffer_length++; \
	out_buffer[out_buffer_length] = '\0'; \
	buffer_full_routine(); \
}

#define in_read(x){ \
	x = in_buffer[in_buffer_pos++];\
	if(in_buffer[in_buffer_pos] == '\0'){ \
		in_buffer_clear();\
	} \
}

#define in_buffer_clear(){ \
	in_buffer_pos = 0; \
	in_buffer[in_buffer_pos] = '\0'; \
}

#define in_call(){ \
	in_buffer_clear(); \
	syscall(SYSCALL_GETS, in_buffer);\
}	

char getc(){
	if(!in_set){
		iobuffer_init();
	}

	if(out_buffer_length > 0){
		out_flush();
	}

	if(in_buffer[in_buffer_pos] != '\0'){
		return in_buffer[in_buffer_pos++];
	}
	
	in_buffer_clear();
	syscall(SYSCALL_GETS, in_buffer);

	return in_buffer[in_buffer_pos++];
}

int gets(char *buff, int size){
	if(buff == NULL || size <= 0){
		return 1;
	}

	if(!in_set){
		iobuffer_init();
	}

	if(out_buffer_length > 0){
		out_flush();
	}

	if(in_buffer_pos == 0){
		syscall(SYSCALL_GETS, in_buffer);
	}

	char *t = in_buffer;
	int i = 0;
	while(i < size && *t != '\0'){
		in_read(buff[i++]);
		t++;
	}

	// while(i < size){
	// 	if(*t == '\0'){
	// 		in_buffer_clear();
	// 	}
	// }
	// in_buffer_clear();

	buff[i] = '\0';

	return 0;
}


int iobuffer_init(){
	out_buffer[0] = '\0';
	out_buffer_length = 0;
	out_set = 1;

	in_buffer[0] = '\0';
	in_buffer_length = 0;
	in_buffer_pos = 0;
	in_set = 1;

	return 0;
}

// scan part

int scan_int(int *ptr, char *last){
    if(ptr == NULL)
        return 0;
    
    *ptr = 0;
    
    int numlen = 0;
    char t = getc();
    while(isspace(t))
        t = getc();
        
    int minus = 1;
    if(t == '-'){
        minus = -1;
        t = getc();
    }
    while((isdigit(t) || (isspace(t) && numlen == 0)) && numlen < 10){
        if(numlen > 0) *ptr *= 10;
        
        *ptr += t - '0';
        numlen++;
        t = getc();
    }
    
    *last = t;
    
    *ptr *= minus;
    
    return numlen > 0;
}


int scan_double(double *ptr){
    if(ptr == NULL)
        return 0;
    
    *ptr = 0;
    
    int numlen = 0;
    char t = getc();
    while(isspace(t))
        t = getc();
        
    double minus = 1.0;
    if(t == '-'){
        minus = -1.0;
        t = getc();
    }
    while(isdigit(t) || (numlen == 0 && isspace(t))){
        if(numlen > 0) *ptr *= 10;
        
        *ptr += (double)(t - '0');
        numlen++;
        t = getc();
    }
    if(t == '.'){
        t = getc();
        
        int frac_len = 1;
        while(isdigit(t) && frac_len <= 5){
            
            double dt = (double)(t - '0');
            for(int i = 0; i < frac_len; i++ )
                dt /= 10.0;
            
            *ptr += dt;
            frac_len++;
            
            numlen++;
            t = getc();
        }
        while(isdigit(t))
            t = getc();
    }
    
    *ptr *= minus;
    
    return numlen > 0;
}

int my_scanf(char *format, ...){
    va_list lista;
    va_start(lista, format);
    
    int getted = 0;
    for(int i = 0; *(format + i); i++){
        char c = *(format + i);
        if(c == '%'){
            char type = *(format + ++i);
            
            switch(type){
                case 'd':
                {
                    char gb = 0;
                    getted += scan_int(va_arg(lista, int *), &gb);
                }break;
                
                case 'f': getted += scan_double(va_arg(lista, double *)); break;
                default:;
            }
//          format_i += 2;
        }
    }
    
    va_end(lista);
    
    return getted;
}

// print part

int print_str(char *str){
	char *t = str;
	while(*t != '\0'){
		out_buffer_putc(*t);
		if(*t == '\n'){
			out_flush();
		}
		t++;
	}
	
	return t - str;
}

int print_int(int num){
	int numlen = num == 0;
	int t = num;
	while(t != 0) numlen++, t /= 10;
	
	int showed = 0; 
	
	if(num < 0) {
		out_buffer_putc('-');
		num *= -1;
		showed++;
	}
	
	for(int i = 1; i <= numlen; i++)
	{
		int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		out_buffer_putc((dig % 10) + '0')
		showed++;
	}
	
	return showed;	
}

int print_unsigned_int(int num){
	int numlen = num == 0;
	int t = num;
	while(t != 0) numlen++, t /= 10;
	
	int showed = 0; 
	
	for(int i = 1; i <= numlen; i++)
	{
		unsigned int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		out_buffer_putc((dig % 10) + '0')
		showed++;
	}
	
	return showed;	
}

int print_hex(unsigned int num){
	int showed = 2;
	
	for(int i = 7; i >= 0; i--){
		unsigned int t = (num & (0xF << (i*4))) >> (i*4);
		if(t < 10){
			out_buffer_putc(t + '0');
		}else{
			out_buffer_putc(t - 10 + 'A');
		}
		// num >> 4;
		showed++;
	}
	return showed;
}

int printf(char *format, ...){
	if(!out_set)
		iobuffer_init();
// int printf_func(int dest, char *format, ...){
	va_list lista;
	va_start(lista, format);
	
	int showed = 0;
	for(int i = 0; *(format + i); i++){
		char c = *(format + i);
		if(c == '%'){
			char type = *(format + ++i);
			switch(type){
				case 's': 
				{
					char *str = va_arg(lista, char *);
					showed += print_str(str); 
					// for(int j = 0; str[j] != '\0'; j++){
					// 	if(str[j] == '\n'){
					// 		out_flush();
					// 		break;
					// 	}
					// }
				}
				break;
				case 'i': showed += print_int(va_arg(lista, int)); break;
				case 'd': showed += print_int(va_arg(lista, int)); break;
				case 'u': showed += print_unsigned_int(va_arg(lista, unsigned int)); break;
				case 'x': showed += print_hex(va_arg(lista, int)); break;
			}
		}else{
			out_buffer_putc(c);
			if(c == '\n')
				out_flush();
		}
	}
	
	va_end(lista);

//	term_draw();
	
	return showed;
}