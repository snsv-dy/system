#include "io.h"


void serial_configure_baud_rate(unsigned short com, unsigned short divisor){
	outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
	outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
	outb(SERIAL_DATA_PORT(com), divisor && 0x00FF);
}

void serial_configure_line(unsigned short com){
	outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

void serial_init(){
	serial_configure_baud_rate(SERIAL_COM1_BASE, 3);
	serial_configure_line(SERIAL_COM1_BASE);
}

int is_transmit_empty(){
	return inb(SERIAL_LINE_STATUS_PORT(SERIAL_COM1_BASE)) & 0x20;	
}

void serial_putc(const char c){
	while(is_transmit_empty() == 0);

	outb(SERIAL_DATA_PORT(SERIAL_COM1_BASE), c);
}

void serial_write(const char *str){
	int i = 0;
	while(str[i]){
		serial_putc(str[i]);
		i++;
	}
}

void itoa(int a, char *buff){
	if(!a){
		buff[0] = '0';
		buff[1] = '\0';
	}else{
		unsigned int length = 0;
		int b = a;
		while(b > 0){
			b /= 10;
			length++;
		}

		//framebuffer_charAt(length + '0', 0, 0);

		int i;
		for(i = length - 1; i >= 0; i--){
			buff[i] = (a % 10) + '0';
			a /= 10;	
		}

		buff[length] = '\0';
	}
}

void ultoa(unsigned long a, char *buff){
	if(!a){
		buff[0] = '0';
		buff[1] = '\0';
	}else{
		unsigned int length = 0;
		unsigned long b = a;
		while(b > 0){
			b /= 10;
			length++;
		}

		//framebuffer_charAt(length + '0', 0, 0);

		int i;
		for(i = length - 1; i >= 0; i--){
			buff[i] = (a % 10) + '0';
			a /= 10;	
		}

		buff[length] = '\0';
	}
}


void utoa(unsigned int a, char *buff){
	if(!a){
		buff[0] = '0';
		buff[1] = '\0';
	}else{
		unsigned int length = 0;
		unsigned int b = a;
		while(b > 0){
			b /= 10;
			length++;
		}

		//framebuffer_charAt(length + '0', 0, 0);

		int i;
		for(i = length - 1; i >= 0; i--){
			buff[i] = (a % 10) + '0';
			a /= 10;	
		}

		buff[length] = '\0';
	}
}