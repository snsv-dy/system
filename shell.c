#include "shell.h"


#define FB_COMMAND_PORT	0x3D4
#define FB_DATA_PORT	0x3D5

#define FB_HIGH_BYTE_COMMAND	14
#define FB_LOW_BYTE_COMMAND	15

void framebuffer_move_cursor(unsigned short pos){
	outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
	outb(FB_DATA_PORT, pos >> 8);
	outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
	outb(FB_DATA_PORT, pos & 0x00FF);
}

uint16_t *framebuffer; 
char framebuffer_color;
int framebuffer_width;
int framebuffer_height;
int framebuffer_x;
int framebuffer_y;

char terminal_lines[TERMINAL_HEIGHT][SCREEN_WIDTH] = {0};

// void write_line(char *str, )

void framebuffer_charAt(unsigned char c, int x, int y){
	if(x > framebuffer_width)
		x = 0;
	if(y > framebuffer_height)
		y = 0;

	int pos = ( (y * framebuffer_width) + x);
	// framebuffer[pos] = (c << 8) | framebuffer_color;
	framebuffer[pos] = c | (framebuffer_color << 8);
}

void framebuffer_init(){
	framebuffer = (uint16_t *)0xC00B8000;
	// framebuffer_color = 8<<4 | 15;
	framebuffer_color = 15;
	framebuffer_width = 80;
	framebuffer_height = 25;
	framebuffer_x = 0;
	framebuffer_y = 0;

	terminal_begin = 0;
	terminal_end = 0;
	terminal_full = 0;
	terminal_input_lenght = 0;

	current_line = 0;
	current_char = 0;
	for(int i = 0; i < TERMINAL_HEIGHT; i++)
		for(int j = 0; j < SCREEN_WIDTH; j++)
			terminal_lines[i][j] = ' ';

	terminal_redraw();
}

void dump_term(){
	serial_write(" --- TERM DATA ---");
	for(int i = 0; i < TERMINAL_HEIGHT; i++)
		for(int j = 0; j < SCREEN_WIDTH; j++)
			terminal_lines[i][j] = ' ';
	serial_write(" --- TERM END  ---");
}

unsigned int strlen(char *str){
	unsigned int len = 0;
	while(str[len]) len++;
	return len;
}

void framebuffer_clear(){
	for(int i = 0; i < framebuffer_height; i++)
		for(int j = 0; j < framebuffer_width; j++)
			framebuffer_charAt(' ', j, i);
}

void newline(){
	framebuffer_x = 0;
	framebuffer_y += 1;
	if(framebuffer_y > framebuffer_height)
		framebuffer_y = 0;
}

void putc(char c){
	if(framebuffer_x > framebuffer_width){
		framebuffer_x = 0;
		framebuffer_y++;
	}

	if(framebuffer_y > framebuffer_height){
		framebuffer_y = 0;
	}

	framebuffer_charAt(c, framebuffer_x, framebuffer_y);
	framebuffer_x++;
}

void puts(char *str){
	unsigned int length = strlen(str);
	unsigned int i;
	for(i = 0; i < length; i++){
		putc(str[i]); 
		// framebuffer_move_cursor(81);
	}
	framebuffer_move_cursor((framebuffer_y * framebuffer_width) + framebuffer_x);
}

void terminal_redraw(){
	serial_write("drawing\n");

	// for(int i = terminal_begin, c = 0; 
	for(int i = terminal_begin, c = 0; 
		// (!terminal_full && i < terminal_end) || 
		// (terminal_full && c < SCREEN_HEIGHT)
		c < SCREEN_HEIGHT;
		 i++, c++){
		if( i >= TERMINAL_HEIGHT)
			i = 0;

		for(int j = 0; j < SCREEN_WIDTH; j++){
			// char to_put = terminal_lines[i][j];
			framebuffer_charAt(terminal_lines[i][j], j, c);
		}
	}

	// for(int i = 0; i < SCREEN_HEIGHT; i++){
	// 	for(int j = 0; j < SCREEN_WIDTH; j++){
	// 		framebuffer_charAt(terminal_lines[i][j], j, i);	
	// 	}
	// }

	serial_write("drawing end\n");
	char buf[25];
	itoa(terminal_begin, buf);
	serial_write("[REDRAW] term beg: ");
	serial_write(buf);
	serial_write("\n[REDRAW] term end: ");
	buf[0] = '\0';
	itoa(terminal_begin, buf);
	serial_write(buf);
	serial_write("\n");
}

int terminal_sanity_check(){
	int redraw = 0;

	char buf[24];
	itoa(terminal_end, buf);
	serial_write("term end: ");
	serial_write(buf);
	serial_write("\n");
	// if(current_char >= SCREEN_WIDTH){
		// current_char = 0;

		// terminal_end += 1;
		if(!terminal_full && terminal_end >= SCREEN_HEIGHT)
			terminal_full = 1;

		if(terminal_full){
			terminal_begin += 1;
			redraw = 1;
		}

		if(terminal_end >= TERMINAL_HEIGHT){
			serial_write("TERMINAL_END >= TERMINAL_HEIGHT\n");
			terminal_end = 0;
		}

		if(terminal_begin >= TERMINAL_HEIGHT){
			terminal_begin = 0;
		}

	// itoa(terminal_begin, buf);
	// serial_write("[SANITY] term beg: ");
	// serial_write(buf);
	// serial_write("\n[SANITY] term end: ");
	// itoa(terminal_begin, buf);
	// serial_write(buf);
	// serial_write("\n");
	// }

	return redraw;
}

void terminal_putc(char c){
	char redraw = 0;
	if(current_char >= SCREEN_WIDTH){
		current_char = 0;

		terminal_end += 1;
		redraw = terminal_sanity_check();
		// if(!terminal_full && terminal_end >= SCREEN_HEIGHT)
		// 	terminal_full = 1;

		// if(terminal_full){
		// 	terminal_begin += 1;
		// 	redraw = 1;
		// }

		// if(terminal_end >= TERMINAL_HEIGHT){
		// 	terminal_end = 0;
		// }

		// if(terminal_begin >= TERMINAL_HEIGHT){
		// 	terminal_begin = 0;
		// }
	}

	char charAt_y = terminal_end;
	if(terminal_end >= SCREEN_HEIGHT){
		charAt_y = SCREEN_HEIGHT - 1;
	}

	framebuffer_charAt(c, current_char, charAt_y);

	terminal_lines[terminal_end][current_char] = c;
	// serial_write("putting c: ");
	// serial_putc(terminal_lines[terminal_end][current_char]);
	// serial_write("\n");
	current_char++;

	if(redraw){
		terminal_redraw();
	}

	int cursor_y = terminal_end;
	if(terminal_full){
		cursor_y = SCREEN_HEIGHT - 1;
	}
	framebuffer_move_cursor(SCREEN_WIDTH * cursor_y + current_char);
}

void terminal_puts(char *str){
	for(int i = 0; str[i] != '\0'; i++){
		terminal_putc(str[i]);
	}
	serial_write("TERM: ");
	serial_write(str);
	serial_write("\n");
}

void terminal_enter(){
	terminal_end++;

	current_char = 0;
	if(terminal_sanity_check())
		terminal_redraw();

	for(int i = 0; i < SCREEN_WIDTH; i++)
		terminal_lines[terminal_end][i] = ' ';
	
	int cursor_y = terminal_end;
	if(terminal_full){
		cursor_y = SCREEN_HEIGHT;
	}

	framebuffer_move_cursor(SCREEN_WIDTH * cursor_y + current_char);
	terminal_input_lenght = 0;
}

void terminal_input_from_keyboard(){
	terminal_input_lenght++;	
}

void terminal_backspace(){
	if(terminal_input_lenght > 0){
		terminal_input_lenght--;


		int temp_end = terminal_end;
		if(temp_end >= SCREEN_HEIGHT)
			temp_end = SCREEN_HEIGHT - 1;

		current_char--;
		terminal_lines[terminal_end][current_char] = ' ';
		if(current_char < 0){
			// framebuffer_y = 0; framebuffer_x = 0;
			current_char = SCREEN_WIDTH - 1;

			terminal_end--;
			temp_end--;
			if(terminal_full)
				terminal_begin--;

			serial_write("[TERM] term end: ");
			serial_putc(terminal_end / 10 % 10 + '0');
			serial_putc(terminal_end% 10 + '0');
			serial_putc('\n');

			// for(; terminal_lines[terminal_end][current_char] != '\0' && terminal_lines[terminal_end][current_char] != ' '; current_char++);


			// puts("Current char: ");
			// char buf[12];
			// itoa(current_char, buf);
			// puts(buf);
		}

		if(terminal_end < 0){
			terminal_end = 0;
		}
		terminal_redraw();
		// framebuffer_charAt(' ', current_char, temp_end);

		framebuffer_move_cursor(SCREEN_WIDTH * temp_end + current_char);
	}
}

// void terminal_scroll(TERMINAL_SCROLL_DIR dir){
// 	if(terminal_full){
// 		switch(dir){
// 			case TERMINAL_SCROLL_UP:

// 			break;
// 		}
// 	}
// }

char tolower(const char c){
	if(c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}

char toupper(const char c){
	if(c >= 'A' && c <= 'Z')
		return c;
	return c - 'a' + 'A';
}

char isalpha(char c){
	c = tolower(c);
	return c >= 'a' && c <= 'z';
}

char isnum(char c){
	return c >= '0' && c <= '9'; 
}