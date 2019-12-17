#ifndef _THREAD_H_
#define _THREAD_H_

#include "memory2.h"
#include "heap.h"
#include "util.h"	// CEIL
#include "doubly_linked_list.h"

typedef struct{
	unsigned int magic;							//	0
	unsigned int bits : 8; // 32 bit or 64 bit		4
	unsigned int endianess : 8;					//	5
	unsigned int header_version : 8;			//	6
	unsigned int abi : 8;						//	7
	unsigned char padding[8];					//	15
	unsigned int instruction_set : 16;
	unsigned int elf_version;
	unsigned int program_entry;
	unsigned int program_header_table;
	unsigned int section_header_table;
	unsigned int flags;
	unsigned int header_size : 16;
	unsigned int program_table_entry_size : 16;
	unsigned int program_table_num_entries : 16;
	unsigned int header_table_entry_size : 16;
	unsigned int header_table_num_entries : 16;
	unsigned int section_names_index : 16;
} elf_header;

typedef struct{
	unsigned int type;
	unsigned int offset;
	unsigned int v_mem_start;
	unsigned int abi;
	unsigned int size_in_file;
	unsigned int size_in_memory;
	unsigned int flags;
	unsigned int alignment;
} program_header;

typedef struct{
	unsigned int esp;
	unsigned int ebp;
	unsigned int eip;
	unsigned int privl;
} proc_state;

#define THREAD_STATUS_READY 0
#define THREAD_STATUS_ACTIVE 1
#define THREAD_STATUS_WAITING 2

struct registers_struct{
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int espu;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;

	unsigned int esp;
	unsigned int eip;
	unsigned int privl;	// to nie jest rejestr, ale jest używane w 
};

typedef struct{
	struct page_directory_struct *directory;

	unsigned int stack;	// wskaźnik stosu
	unsigned int code;	// wskaźnik na początek kodu

	unsigned int pid;
	unsigned int status;
	int sleep_pos;	// pozycja w kolejce sleepów (nie inicjowane do czasu wywołania sleepa)
	struct registers_struct saved_registers;
	unsigned int were_registers_saved;	// czy rejestry były zapisywane (nie będą w przypadku kiedy wątek będzie uruchamiany poraz pierwszy)
} thread_t;

void display_state(proc_state *st);

unsigned int *return_int();
void thread_start(thread_t *thread);
thread_t *thread_setup(elf_header *elf, unsigned int *error);
thread_t *get_current_thread();
struct registers_struct *switch_task(unsigned int saved_eip, unsigned int saved_esp);
void queue_thread(thread_t *t);
void put_thread_to_sleep(thread_t *thread, int how_long);
struct registers_struct *timer_handler(unsigned int esp, unsigned int eip);
void save_registers(
	unsigned int edi,
	unsigned int esi,
	unsigned int ebp,
	unsigned int esp,
	unsigned int ebx,
	unsigned int edx,
	unsigned int ecx,
	unsigned int eax
	);
// global save_registers;

#endif