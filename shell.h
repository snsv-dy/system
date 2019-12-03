#ifndef _SHELL_
#define _SHELL_

#include <stdint.h>
#include "io.h"

#define FB_COMMAND_PORT	0x3D4
#define FB_DATA_PORT	0x3D5

#define FB_HIGH_BYTE_COMMAND	14
#define FB_LOW_BYTE_COMMAND	15

void framebuffer_move_cursor(unsigned short pos);

uint16_t *framebuffer; 
char framebuffer_color;
int framebuffer_width;
int framebuffer_height;
int framebuffer_x;
int framebuffer_y;

// void write_line(char *str, )

void framebuffer_charAt(unsigned char c, int x, int y);
void framebuffer_init();
unsigned int strlen(char *str);
void framebuffer_clear();
void newline();
void puts(char *str);
void putc(char c);

// przewijanie terminala nie działa dobrze,
//	ale narazie przynajmniej wyświetla

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define TERMINAL_HEIGHT 50

int terminal_begin;
int terminal_end;
int terminal_full;
int terminal_input_lenght;

int current_line;
int current_char;
void terminal_write();
void terminal_redraw();
void terminal_enter();
void terminal_puts(char *str);
void terminal_putc(char c);
int terminal_sanity_check();
void terminal_input_from_keyboard();
void terminal_backspace();


typedef enum{
	TERMINAL_SCROLL_DOWN,
	TERMINAL_SCROLL_UP
} TERMINAL_SCROLL_DIR;

// #define TERMINAL_SCROLL_DOWN 	0
// #define TERMINAL_SCROLL_UP 		1

// move this to ctype
char tolower(const char c);
char toupper(const char c);
char isalpha(const char c);
char isnum(char c);


#endif