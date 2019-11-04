#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "io.h"
// #include "shell.h"
#include "gdt.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

extern void flush_tlb();
extern void invl_pg(unsigned int *addr);


typedef struct{
	unsigned int present : 1;
	unsigned int read_write : 1;
	unsigned int user_supervisor : 1;
	unsigned int write_trough : 1;
	unsigned int cache_disable : 1;
	unsigned int accessed : 1;
	unsigned int O : 1;
	unsigned int page_size : 1;
	unsigned int G : 1;
	unsigned int : 3;
	unsigned int page_frame_addr : 20;
} page_dir_entry;

typedef struct{
	unsigned int present : 1;
	unsigned int read_write : 1;
	unsigned int user_supervisor : 1;
	unsigned int write_trough : 1;
	unsigned int cache_disable : 1;
	unsigned int accessed : 1;
	unsigned int dirty : 1;
	unsigned int O : 1;
	unsigned int G : 1;
	unsigned int : 3;
	unsigned int phys_addr : 20;
} page_frame_entry;


unsigned int *boot_page_directory;
unsigned int *get_page_dir();

#define KERNEL_LOADED_ADDR 0xC0000000

unsigned int *page_dir = 0xFFFFF000;
unsigned int page1[1024] __attribute__((aligned (4096)));

void kernel_main(unsigned int kernel_start, unsigned int kernel_end, unsigned int page_addr){

	char buff[20];
	// unsigned int kernel_page = 0xC0000000 >> 22;

	// setting up the pages
	unsigned int page1_base = 4096 * 1024;
	for(int i = 0; i < 1024; i++){
		page1[i] = page1_base | 0x03;
		page1_base += 4096;
	}

	unsigned int table0entry = ((unsigned int)page1 - KERNEL_LOADED_ADDR) | 0x03;
	page_dir[2] = table0entry;

	// invl_pg(0);

	flush_tlb();

	serial_init();
	init_gdt();
	init_idt();
	framebuffer_init();

	utoa(page_dir[2] & 3, buff);
	terminal_puts("dir[1]: ");
	terminal_puts(buff);
	terminal_enter();	

	unsigned int *page1_get = 0xFFC00000 | (2 << 12);
	utoa(page1_get[0] & 3, buff);
	terminal_puts("dir[1][0]: ");
	terminal_puts(buff); 
	terminal_enter();

	utoa(page1_get[0] & ~0x3FF, buff);
	terminal_puts("dir[0][0]: ");
	terminal_puts(buff); 
	terminal_enter();



	unsigned int *ptr = 2 << 22;
	ptr[0] = 5;

	utoa(ptr[0], buff);
	terminal_puts("ptr: ");
	terminal_puts(buff);
	terminal_enter();

	// unsigned char *page1_addr1 = 0x00400000;

	// terminal_puts("Write to allocated: ");
	// page1_addr1[0] = 'a';
	// terminal_enter();

	// terminal_puts("Read from allocated: ");
	// utoa(page1_addr1[0], buff);
	// terminal_puts(buff);
	// terminal_enter();


	// unsigned int page1phys = 0x00400000; // 4mb od początku pamięci
	// for(unsigned int i = 0; i < 1024; i++){
	// 	if(i == kernel_page){
	// 		*(unsigned int *)(pd.page_directory + i) = 0;
	// 		pd.page_directory[i].present = 1;
	// 		pd.page_directory[i].write_trough = 1;
	// 		pd.page_directory[i].page_size = 1;
	// 		pd.page_directory[i].page_frame_addr = 0x00000000;
	// 	}else if(i == 1){
	// 		*(unsigned int *)(pd.page_directory + i) = 0;

	// 		for(int j = 0; j < 1024; j++){
	// 			*(unsigned int *)(pd.pf1 + j) = 0;
	// 			pd.pf1[j].present = 1;
	// 			pd.pf1[j].write_trough = 1;
	// 			pd.pf1[j].phys_addr = page1phys + j * 4096 - KERNEL_LOADED_ADDR;
	// 		}

	// 		pd.page_directory[i].present = 1;
	// 		pd.page_directory[i].write_trough = 1;
	// 		pd.page_directory[i].page_frame_addr = &(pd.pf1) - KERNEL_LOADED_ADDR;
	// 	}else{
	// 		*(unsigned int *)(pd.page_directory + i) = 0;
	// 	}
	// }

	// will be moved to separate file
	//set_page_directory((unsigned int)(&pd.page_directory) - KERNEL_LOADED_ADDR);
}