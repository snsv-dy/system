#ifndef _GDT_H_
#define _GDT_H_

#include "io.h"
#include "shell.h"
#include "memory2.h"
#include "stdio.h"
#include "thread.h"

// GDT part
#define GDT_RING3_CODE_SELECTOR 24
#define GDT_RING3_DATA_SELECTOR 32

void gdt_flush(unsigned int);
void tss_flush();

// struct gdt_entry_s{
// 	unsigned short limit_low;
// 	unsigned short base_low;
// 	unsigned char base_middle;
// 	unsigned char access;
// 	unsigned char granularity;
// 	unsigned char base_high;
// } __attribute__((packed));
// typedef struct gdt_entry_s gdt_entry;

struct tss_entry_struct{
	unsigned int prev_tss;
	unsigned int esp0;
	unsigned int ss0;
	unsigned int esp1;
	unsigned int ss1;
	unsigned int esp2;
	unsigned int ss2;
	unsigned int cr3;
	unsigned int eip;
	unsigned int eflags;
	unsigned int eax;
	unsigned int ecx;
	unsigned int edx;
	unsigned int evx;
	unsigned int esp;
	unsigned int ebp;
	unsigned int esi;
	unsigned int edi;
	unsigned int es;
	unsigned int cs;
	unsigned int ss;
	unsigned int ds;
	unsigned int fs;
	unsigned int gs;
	unsigned int ldt;
	unsigned short trap;
	unsigned short iomap_base;
}__attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;

struct gdt_entry_s{
	unsigned int limit_low : 16;
	unsigned int base_low : 24;
	// atrybuty
	unsigned int accessed : 1;
	unsigned int read_write : 1;
	unsigned int conforming_expand_down : 1;
	unsigned int executable : 1; // 1 jeżeli jest to segment kodu (czyli wykonywalny)
	unsigned int desc_type : 1; // 1 dla segmentu kodu i danych, 0 dla task state
	unsigned int privl : 2; // w którym ringu jest segment
	unsigned int present : 1;
	// dalsza część limitu
	unsigned int limit_high : 4;
	// flagi
	// unsigned int available : 1;
	unsigned int always_0 : 2;
	unsigned int big : 1;
	unsigned int granularity : 1;
	unsigned int base_high : 8;
} __attribute__((packed));
typedef struct gdt_entry_s gdt_entry;

struct gdt_ptr_s{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));
typedef struct gdt_ptr_s gdt_ptr;


//IDT part

struct idt_entry_s{
	unsigned short offset_low;
	unsigned short selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short offset_high;
} __attribute__((packed));
typedef struct idt_entry_s idt_entry;

extern int load_idt();
extern int irqdefm();
extern int irq1();
extern int irqdefs();
extern int page_except();
extern int double_fault();
extern int syscall();
extern int gpf(); // general protection fault
extern void halt();
extern int timer_interrupt();

#define KEYBOARD_DATA_PORT 0x60

gdt_entry *init_gdt();
void set_kernel_stack(unsigned int stack);

void init_pit();
void init_idt(); 
void irqmaster_handler();
void irq1_handler();
void irqslave_handler();
extern struct registers_struct *timer_handler(unsigned int esp, unsigned int eip);
void page_except_handler(unsigned int vaddr, unsigned int err);
void *syscall_handler(unsigned int param1, unsigned int param2, unsigned int prog_esp, unsigned int esp, unsigned int eip);

void init_key_map();

#endif