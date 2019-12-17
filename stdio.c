#include "stdio.h"


int print_text(char *str){
	int showed = 0;
	for(int i = 0; *(str + i); i++)
		terminal_putc(*(str + i)), showed++;
	return showed;
}

int print_int(int num){
	int numlen = num == 0;
	int t = num;
	while(t != 0) numlen++, t /= 10;
	
	int showed = 0; 
	
	if(num < 0) {
		terminal_putc('-');
		num *= -1;
		showed++;
	}
	
	for(int i = 1; i <= numlen; i++)
	{
		int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		terminal_putc((dig % 10) + '0'), showed++;;
	}
	
	return showed;
}

int serial_d(int num){
	int numlen = num == 0;
	int t = num;
	while(t != 0) numlen++, t /= 10;
	
	int showed = 0; 
	
	if(num < 0) {
		serial_putc('-');
		num *= -1;
		showed++;
	}
	
	for(int i = 1; i <= numlen; i++)
	{
		int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		serial_putc((dig % 10) + '0'), showed++;;
	}
	
	return showed;	
}

int print_unsigned(unsigned int num){
	int numlen = num == 0;
	unsigned int t = num;
	while(t != 0) numlen++, t /= 10;
	
	int showed = 0; 
	
	if(num < 0) {
		terminal_putc('-');
		num *= -1;
		showed++;
	}
	
	for(int i = 1; i <= numlen; i++)
	{
		unsigned int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		terminal_putc((dig % 10) + '0'), showed++;;
	}
	
	return showed;
}

int print_uhex(unsigned int num){
	int showed = 2;

	terminal_putc('0');
	terminal_putc('x');
	for(int i = 7; i >= 0; i--){
		unsigned int t = (num & (0xF << (i*4))) >> (i*4);
		if(t < 10)
			terminal_putc(t + '0');
		else
			terminal_putc(t - 10 + 'A');
		// num >> 4;
		showed++;
	}
	return showed;
}

int print_double(double num){
	int numlen = num == 0;
	double t = num;
	
	double a = num;
	if(a < 0) a = -a;
	
	
	while((int)t != 0) {
		numlen++;
		t /= 10;
	}
	
//	printf("numlen: %d\n", len);
	int showed = 0;
	
	if(num < 0) {
		terminal_putc('-');
		num *= -1;
		showed++;
	}
	
	//wyświetlanie cyfr przed kropką
	for(int i = 1; i <= numlen; i++){
		int dig = num;
		for(int j = 0; j < numlen - i; j++){
			dig /= 10;
		}
		
		terminal_putc(((int)dig % 10) + '0');
	}
	showed += numlen;
	
	terminal_putc('.');
	showed++;
//	printf("isnum<0: %d\n", num < 0);
	num -= (int)num;
	for(int i = 0; i < 5; i++){
		num *= 10;
		
		terminal_putc(((int)num % 10) + '0');
	}
	showed += 5;
	
	return showed;
}

void serial_x(unsigned int num){

	serial_putc('0');
	serial_putc('x');
	for(int i = 7; i >= 0; i--){
		unsigned int t = (num & (0xF << (i*4))) >> (i*4);
		if(t < 10)
			serial_putc(t + '0');
		else
			serial_putc(t - 10 + 'A');
	}
}

int printf(char *format, ...){
// int printf_func(int dest, char *format, ...){
	va_list lista;
	va_start(lista, format);
	
	int showed = 0;
	for(int i = 0; *(format + i); i++){
		char c = *(format + i);
		if(c == '%'){
			char type = *(format + ++i);
			switch(type){
				case 's': showed += print_text(va_arg(lista, char *)); break;
				case 'd': showed += print_int(va_arg(lista, int)); break;
				case 'u': showed += print_unsigned(va_arg(lista, unsigned int)); break;
				case 'f': showed += print_double(va_arg(lista, double)); break;
				case 'x': showed += print_uhex(va_arg(lista, unsigned int)); break;
			}
		}else if(c =='\n'){
			terminal_enter();
		}else{
			terminal_putc(c);
			showed++;
		}
	}
	
	va_end(lista);
	
	return showed;
}