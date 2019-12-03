#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "io.h"
#include "shell.h"
#include "stdio.h" 
#include "gdt.h"
#include "multiboot.h"
#include "memory2.h" 

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif 
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

extern unsigned int *_kernel_start;
extern unsigned int *_kernel_end;

// extern void *kernel_heap;
void kernel_main(
	unsigned int kernel_start,
	unsigned int kernel_end,
	multiboot_info_t *mbi, 
	unsigned int magic ){

	init_gdt();
	init_idt();
	serial_init();
	framebuffer_init();

	multiboot_memory_map *mem_map = (multiboot_memory_map *)mbi->mmap_addr;
	unsigned int mem_map_size = mbi->mmap_length;

	initialize_memory(kernel_start, kernel_end, mem_map, mem_map_size);

	printf("Thats it\n");
}