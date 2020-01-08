#ifndef _SHELL_
#define _SHELL_

#include <stdint.h>
#include "io.h"
#include "util.h"

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

void framebuffer_charAt(unsigned char c, int x, int y);
void framebuffer_init();
// unsigned int strlen(char *str);
void framebuffer_clear();
void newline();

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define TERM_WIDTH SCREEN_WIDTH
#define TERM_HEIGHT 30
#define TERMINAL_HEIGHT 50

int terminal_begin;
int terminal_end;
int terminal_full;
int terminal_input_lenght;

int current_line;
int current_char;
int terminal_sanity_check();
void terminal_input_from_keyboard();

typedef enum{
	TERMINAL_SCROLL_DOWN,
	TERMINAL_SCROLL_UP
} TERMINAL_SCROLL_DIR;

void term_putc(char c, int ref);
void term_write(char *str);
void term_draw();
void term_enter(int org);
void term_clear_line(int line);
void term_backspace();
void term_scroll(TERMINAL_SCROLL_DIR dir);
int term_keyboard_in(char c);
char *term_buff();

// #define terminal_write(x)

#define terminal_puts(x) term_write(x)
#define terminal_redraw term_draw
#define terminal_putc(c) term_putc(c, 1)
#define terminal_backspace term_backspace

#define TERM_NOREFRESH 0
#define TERM_REFRESH 1

// move this to ctype
char tolower(const char c);
char toupper(const char c);
char isalpha(const char c);
char isnum(char c);

void add_prog_name(char *name);
void term_thread();


#endif