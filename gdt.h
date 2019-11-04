#ifndef _GDT_H_
#define _GDT_H_

#include "io.h"
#include "shell.h"

// GDT part

void gdt_flush(unsigned int);

struct gdt_entry_s{
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char base_middle;
	unsigned char access;
	unsigned char granularity;
	unsigned char base_high;
} __attribute__((packed));
typedef struct gdt_entry_s gdt_entry;

struct gdt_ptr_s{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));
typedef struct gdt_ptr_s gdt_ptr;

void init_gdt();

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

#define KEYBOARD_DATA_PORT 0x60

void init_idt();
void irqmaster_handler();
void irq1_handler();
void irqslave_handler();
void page_except_handler(unsigned int vaddr, unsigned int err);

void init_key_map();

#endif