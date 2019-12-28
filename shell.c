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

int term_size = TERM_HEIGHT * SCREEN_WIDTH;
char term_data[TERM_HEIGHT * SCREEN_WIDTH] = {0};

#define TERM_AT(x, y) term_data[y * TERM_WIDTH + x]

int term_view_beg = 0;
int term_view_end = SCREEN_HEIGHT - 1;
int term_end = 0;
int term_full = 0;
// void write_line(char *str, )

#define IN_BUFFER_LENGTH 1000
char in_buffer[IN_BUFFER_LENGTH];
int in_buffer_pos;
int input_interrupted; // czy podzczas wprowadzania danych, został wyświetlony jakiś inny tekst
int in_read;	// czy bufor został odczytany przez program i może być wyczyszczony

#define in_buffer_clear(){ \
	in_buffer_pos = 0; \
	in_buffer[in_buffer_pos] = '\0'; \
}

#define in_buffer_putc(c) { \
	in_buffer[in_buffer_pos++] = c; \
	in_buffer[in_buffer_pos] = '\0'; \
}

#define in_buffer_backspace() { \
	if(in_buffer_pos > 0){ \
		in_buffer[in_buffer_pos - 1] = '\0'; \
	} \
}

char *term_buff(){
	in_read = 1;
	return in_buffer;
}

int term_keyboard_in(char c){
	if(in_read){
		in_buffer_clear();
		in_read = 0;
	}

	int end_of_input = 0; // czy został wciśnięty enter i można przekazać tekst programowi


	if(input_interrupted){
		term_write(in_buffer);
	}
	
	if(c != '\b')
		in_buffer_putc(c);

	if(c == '\n'){
		end_of_input = in_buffer_pos;
		term_enter(1);
	}else if(c == '\b'){
		term_backspace();
	}else{
		term_putc(c, TERM_NOREFRESH);
	}

	term_draw();
	input_interrupted = 0;

	return end_of_input;
}


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
	// framebuffer = (uint16_t *)0xC00B8000;
	framebuffer = (uint16_t *)0x000B8000;
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

	in_buffer_pos = 0;
	in_buffer[0] = '\0';
	input_interrupted = 0;
	in_read = 0;

	current_line = 0;
	current_char = 0;
	for(int i = 0; i < TERMINAL_HEIGHT; i++)
		for(int j = 0; j < SCREEN_WIDTH; j++)
			TERM_AT(j, i) = ' ';

	term_draw();
}

void dump_term(){
	serial_write(" --- TERM DATA ---");
	for(int i = 0; i < TERMINAL_HEIGHT; i++)
		for(int j = 0; j < SCREEN_WIDTH; j++)
			TERM_AT(j, i) = ' ';
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

void term_draw(){
	for(int i = term_view_beg, c = 0; c < SCREEN_HEIGHT; i++, c++){
		if( i >= TERM_HEIGHT)
			i = 0;

		for(int j = 0; j < TERM_WIDTH; j++){
			framebuffer_charAt(TERM_AT(j, i), j, c);
		}
	}

	int last_line = (term_end / TERM_WIDTH);
	int line_end = term_end % TERM_WIDTH;
	int bottom = last_line;//term_view_end / TERM_WIDTH;
	if(line_end > SCREEN_HEIGHT - 1)
		bottom = SCREEN_HEIGHT - 1;

	framebuffer_move_cursor(bottom * TERM_WIDTH + line_end);
}

#define is_term_scroll() \
 ( (term_end % TERM_WIDTH == 0 && term_full) || ((term_end / TERM_WIDTH) > term_view_end))


void term_putc(char c, int ref){
	term_data[term_end++] = c;

	if(term_end >= term_size){
		term_end = 0;
		term_full = 1;
	}

	if(is_term_scroll()){
		term_scroll(TERMINAL_SCROLL_DOWN);
		
		term_clear_line(term_view_end);
	}

	if(ref)
		term_draw();

	input_interrupted = 1;
}
#define terminal_putc(c) term_putc(c, 1)

void term_write(char *str){
	while(*str != '\0'){
		term_putc(*str, 0);
		str++;
	}
	term_draw();
}

void term_enter(int org){
	term_end += TERM_WIDTH - term_end % TERM_WIDTH;
	if(term_end >= term_size){
		term_full = 1;
		term_end = 0;
	}
	if(is_term_scroll()){
		term_scroll(TERMINAL_SCROLL_DOWN);
		term_clear_line(term_view_end);
	}
}

void term_clear_line(int line){
	line = line % TERM_HEIGHT;
	for(int i = 0; i < TERM_WIDTH; i++){
		TERM_AT(i, line) = ' ';
	}
}

void term_backspace(){
	
	if(in_buffer_pos > 0){
		serial_write("in_buffer_pos: ");
		serial_d(in_buffer_pos);
		serial_write("\n");

		in_buffer_backspace();
		in_buffer_pos--;

		if(!input_interrupted){
			term_end--;
			term_data[term_end] = ' ';
		}else{
			term_write(in_buffer);
		}
		term_draw();
	}
}

void term_scroll(TERMINAL_SCROLL_DIR dir){
	switch(dir){
		case TERMINAL_SCROLL_UP:
		{
			term_view_beg--;
			if(term_view_beg < 0)
				term_view_beg = TERM_HEIGHT - 1;
				
			term_view_end--;
			if(term_view_end < 0)
				term_view_end = TERM_HEIGHT - 1;
		}
		break;
		case TERMINAL_SCROLL_DOWN:
		{
			term_view_beg++;
			if(term_view_beg >= TERM_HEIGHT)
				term_view_beg = 0;
				
			term_view_end++;
			if(term_view_end >= TERM_HEIGHT)
				term_view_end = 0;
		}
		break;
	}
}

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