#ifndef _THREAD_H_
#define _THREAD_H_

#include "memory2.h"
#include "heap.h"
#include "util.h"	// CEIL
#include "doubly_linked_list.h"
#include <stdarg.h>

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

enum Queue{
	THREAD,
	SLEEP,
	DEATH,
	IO,
	WAIT
};

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

#define THREAD_STACK_SIZE (8192 * 1024)
#define MAX_THREADS 127	// 1GB / 8Mb - 1, -1 bo trzeba odjąć stos wątku głównego

typedef struct thread_t_struct{
	struct page_directory_struct *directory;

	unsigned int stack;	// wskaźnik stosu
	unsigned int code;	// wskaźnik na początek kodu

	unsigned int pid;	// pid FFFFFFFF, to wątek widmo, nie posiada katalogu, ale nie został usunięty dla zachowania poprawności kolejki śpiących wątków
	unsigned int status;
	enum Queue queue;

	int sleep_pos;	// pozycja w kolejce sleepów (nie inicjowane do czasu wywołania sleepa)
					// albo adres bufora wejścia wątku
					// albo id semafora nienazwanego na którym czeka wątek
	struct registers_struct saved_registers;
	unsigned int were_registers_saved;	// czy rejestry były zapisywane (nie będą w przypadku kiedy wątek będzie uruchamiany poraz pierwszy)

	struct thread_t_struct *parent; // jeżeli parent != NULL to ten wątek został stworzony przez thread_create, jeżeli parent == NULL wątek jest samodzielny
	
	struct thread_t_struct **children;
	unsigned int nchilds;
	unsigned int children_capacity;
} thread_t;

void display_state(proc_state *st);

int init_stack(thread_t *thread, int thread_num);
unsigned int *return_int();
void thread_start(thread_t *thread);
int free_whole_program(struct page_directory_struct *directory);
thread_t *thread_setup(elf_header *elf, int pid, unsigned int *error);
thread_t *get_current_thread();
struct registers_struct *switch_task(unsigned int saved_eip, unsigned int saved_esp);
void queue_thread(thread_t *t);
void put_thread_to_sleep(thread_t *thread, int how_long);
struct registers_struct *timer_handler(unsigned int esp, unsigned int eip);
int get_next_pid();
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

struct page_directory_struct *clone_directory(struct page_directory_struct *src, unsigned int *error);

enum spawn_type{
	CREATE_THREAD,		// dodaje nowy stos w tym samym katalogu stron, i zaczyna wykonywanie od funkcji przekazanej w parametrze
	CREATE_PROCESS		// ładuje program z pamięci 
};

typedef struct {
	thread_t *parent;
	unsigned int new_pid;

	enum spawn_type type;

	// dane do nowego wątku
	void *thread_ptr;
	unsigned int eip; // wskaźnik na funckję wywołującą docelową funkcję
	void *(*func)(void *param);
	void *param;

} spawn_t;

int push_spawn(spawn_t *t);
void thread_spawner();
void thread_killer();

int push_io(thread_t *t);
thread_t *pop_io();

// struktura semafora nienazwanego
typedef struct{
	int counter;
	int id;
	char *name;
} sem_t;

thread_t *get_waiting();
int push_wait(thread_t *t);

// Definicje używane do określenia na co wątek czeka w kolejce czekających wątków
#define WAIT_SEM	0	// czeka na semafora
#define WAIT_PROC	1	// czeka na zakończenie innego wątku

extern void disable_interrupts();
extern void enable_interrupts();

#endif