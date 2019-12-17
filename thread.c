#include "thread.h"

extern struct page_directory_struct kernel_directory_struct;
extern unsigned int *some_program;

extern void interrupt_return(
	unsigned int eip,
	unsigned int esp,
	unsigned int privl);

// thread_t *current_thread = NULL;


struct list_info *thread_queue = NULL;	// Kolejka wątków gotowych do wykonania
struct list_info *sleep_queue = NULL;	// Lista śpiących wątków
struct list_info *wait_queue = NULL;	// Lista wątków czekających na jakiś sygnał lub dane

unsigned int switching_go = 0;

unsigned int next_pid = 1;

thread_t *get_current_thread(){
	if(thread_queue == NULL)
		return NULL;

	return (thread_t *)(thread_queue->head->value);
}

unsigned int get_privl_level(){
	thread_t *cur_thread = get_current_thread();
	if(cur_thread != NULL){
		return cur_thread->saved_registers.privl;
	}

	return 4; // nie ma takiego poziomu, więc coś jest nie tak (na razie nie sprawdzam tego przypadku)
}

void save_registers(unsigned int edi, unsigned int esi, unsigned int ebp, unsigned int esp, unsigned int ebx, unsigned int edx, unsigned int ecx, unsigned int eax){
	thread_t *cur_thread = get_current_thread();
	if(cur_thread != NULL){
		cur_thread->saved_registers.edi = edi;
		cur_thread->saved_registers.esi = esi;
		cur_thread->saved_registers.ebp = ebp;
		cur_thread->saved_registers.espu = esp;
		cur_thread->saved_registers.ebx = ebx;
		cur_thread->saved_registers.edx = edx;
		cur_thread->saved_registers.ecx = ecx;
		cur_thread->saved_registers.eax = eax;
		cur_thread->were_registers_saved = 1;
	}else{
		printf("[SAVE REGISTERS] Cur thread == NULL\n");
	}
}

struct registers_struct *load_registers(){
	thread_t *cur_thread = get_current_thread();
	printf("loading registers\n");
	if(cur_thread != NULL && cur_thread->were_registers_saved){
		return &(cur_thread->saved_registers);
	}

	return NULL;
}

void put_thread_to_sleep(thread_t *thread, int how_long){
	if(thread != NULL && sleep_queue != NULL){
		thread->sleep_pos = how_long;
		// printf("[THREADS] Queing thread %d, pos: %d\n", queue_num(sleep_queue, thread), thread->sleep_pos);
		queue_num(sleep_queue, thread);
		// queue_num(sleep_queue, thread);	
	}
	if(sleep_queue == NULL){
		// printf("sleep_queue == NULL\n");
	}
}

int *thread_sleep_getter(void *thread){
	if(thread == NULL)
		return -1;

	thread_t *t = (thread_t *)thread;
	return &t->sleep_pos;
}

#define TIME_INTERVAL 100
int ticks_sience_start = 0;

struct registers_struct *timer_handler(unsigned int eip, unsigned int esp){
	// printf("[TIMER] tick\n");

	ticks_sience_start++;
	if(ticks_sience_start % TIME_INTERVAL == 0){
		// sekunda mineła
	}

	if(!switching_go || thread_queue == NULL || sleep_queue == NULL){
		// outb(0x20, 0x20);
		return NULL;
	}

	int res = queue_dec(sleep_queue);

	while(sleep_queue->head != NULL && ((thread_t *)(sleep_queue->head->value))->sleep_pos == 0){
		thread_t *t = pop_front(sleep_queue);

		if(t != NULL){
			t->status = THREAD_STATUS_READY;
			push_back(thread_queue, t);
		}
	}

	thread_t *cur_thread = get_current_thread();
	struct registers_struct *cur_state = NULL;

	if(cur_thread != NULL){
		cur_state = &(cur_thread->saved_registers);
		cur_state->eip = eip;
		cur_state->esp = esp;
	}

	cur_state = switch_task(esp, eip);
	// outb(0x20, 0x20);

	return cur_state;
}

#define THREAD_QUEUE_SIZE 5 
#define SLEEP_SIZE THREAD_QUEUE_SIZE

int init_multitasking(thread_t *thread){
	if(thread == NULL)
		return 1;

	unsigned int queue_mem_size = sizeof(struct list_info) + sizeof(struct lnode) * THREAD_QUEUE_SIZE;
	char *queue_mem = (char *)kmalloc(queue_mem_size);
	if(queue_mem == NULL)
		return 2;
	struct list_info *tq = init_list(queue_mem, queue_mem_size);
	if(tq == NULL){
		kfree(queue_mem);
		return 3;
	}

	unsigned int sleep_queue_size = queue_mem_size;
	char *sleep_queue_mem = (char *)kmalloc(queue_mem_size);
	if(sleep_queue_mem == NULL){
		kfree(queue_mem);
		return 4;
	}
	struct list_info *sq = init_list(sleep_queue_mem, sleep_queue_size);
	if(sq == NULL){
		kfree(queue_mem);
		kfree(sleep_queue_mem);
		return 5;
	}

	tq->get_num = thread_sleep_getter;
	sq->get_num = thread_sleep_getter;

	if(push_back(tq, (void *)thread) != 0)
		return 4;

	thread_queue = tq;
	sleep_queue = sq;

	return 0;
}

#define pop_thread() (thread_t *)pop_front(thread_queue)
#define push_thread(x) push_back(thread_queue, x)
#define to_thread(x) (thread_t *)(x)

void queue_thread(thread_t *t){
	push_thread(t);
	printf("QUEUED: %d\n", t->pid);
}

proc_state *sleep_task(unsigned int saved_eip, unsigned int sqved_esp, unsigned int num_ticks){
	if(thread_queue == NULL)
		return NULL;

	thread_t *current_thread = pop_thread();
	current_thread->status = THREAD_STATUS_WAITING;

	put_thread_to_sleep(current_thread, num_ticks);

	thread_t *next_thread = (thread_t *)(thread_queue->head->value);
	if(next_thread == NULL)
		return NULL;
	

	next_thread->status = THREAD_STATUS_ACTIVE;

	struct registers_struct *next_state = &(next_thread->saved_registers);
	set_page_directory(next_thread->directory->directory_addr);

	return &next_thread->saved_registers;
}

struct registers_struct *switch_task(unsigned int saved_eip, unsigned int saved_esp){
	if(thread_queue == NULL)
		return NULL;

	thread_t *current_thread = pop_thread();

	current_thread->status = THREAD_STATUS_READY;
	push_thread(current_thread);


	thread_t *next_thread = (thread_t *)thread_queue->head->value;
	next_thread->status = THREAD_STATUS_ACTIVE;

	set_page_directory(next_thread->directory->directory_addr);
	// printf("[SWITCH TASK] next_pid: %d, next_esp: %x, next_eip: %x\n", next_thread->pid, next_thread->saved_registers.esp, next_thread->saved_registers.eip);

	return &next_thread->saved_registers;
}

void thread_start(thread_t *thread){
	if(thread != NULL){
		thread->status = THREAD_STATUS_ACTIVE;
		set_page_directory(thread->directory->directory_addr);
		// enter_protected(thread->stack, thread->code);
		printf("Starting thread: esp: %x, eip: %x, priv: %d\n", thread->saved_registers.esp, thread->saved_registers.eip, thread->saved_registers.privl);
		switching_go = 1;
		extern unsigned int do_switching;
		do_switching = 1;
		interrupt_return(thread->saved_registers.esp, thread->saved_registers.eip, thread->saved_registers.privl);
	}
}

#define error_check(var, code) \
	if(var == NULL){ \
		if(error != NULL) \
			*error = code; \
		return NULL; \
	}

// errp - error pointer
#define errp(error, code) \
	if(error != NULL) \
		*error = code;

unsigned int *return_int(){
	unsigned int *t = kmalloc(sizeof(unsigned int));
	if(t == NULL){
		printf("t == NULL, couldn't return int\n");
	}
	return t;
}

void display_state(proc_state *st){
	if(st != NULL){
		printf("saved state\n-------\n");
		printf("esp: %x\n", st->esp);
		printf("ebp: %x\n", st->ebp);
		printf("eip: %x\n", st->eip);
	}
}

struct page_directory_struct *clone_directory(struct page_directory_struct *src, unsigned int *error){
	if(src == NULL){
		if(error != NULL)
			*error = 1;
		return NULL;
	}

	struct page_directory_struct *t = (struct page_directory_struct *)kmalloc(sizeof(struct page_directory_struct));
	error_check(t, 2)

	unsigned int *v_bitmap = (unsigned int *)kmalloc(PAGE_BITMAP_LENGTH * sizeof(unsigned int));
	if(v_bitmap == NULL){
		kfree(t);
		errp(error, 3)
		return NULL;
	}

	for(int i = 0; i < PAGE_BITMAP_LENGTH; i++)
		v_bitmap[i] = src->v_bitmap[i];

	// alokowanie za jednym razem pamięć na tablicę używaną w cr3 i tą z adresami wirtualnymi tabel
	unsigned int *dir_p = (unsigned int *)kmalloc_page_aligned(0x2000);
	if(dir_p == NULL){
		kfree(t);
		kfree(v_bitmap);
		errp(error, 4)
		return NULL;
	}
	unsigned int *dir_v = dir_p + 1024;

	for(int i = 0; i < 1024; i++){
		dir_p[i] = src->directory_addr[i];
		dir_v[i] = src->directory_v[i];
	}

	t->directory_addr = dir_p;
	t->directory_v = dir_v;
	t->num_tables = src->num_tables;
	t->v_bitmap = v_bitmap;

	// printf("[CLONE DIR] t = %x\n", t);

	return t;
}


#define free_dir_struct(dir) \
	if(dir != NULL) {\
		if(dir->directory_addr != NULL) \
			kfree(dir->directory_addr); \
		if(dir->directory_v != NULL) \
			kfree(dir->directory_v); \
		kfree(dir); \
	}

#define AFTER_KERNEL 0x40000000

// zwalnia strony fizyczne i tabele
int free_program_pages(struct page_directory_struct *directory){
	if(directory == NULL)
		return 1;

	// Zwalnianie danych i sterty
	int bitmap_pos = AFTER_KERNEL / 0x1000;
	int bit_len = 0;
	while(check_bit(directory->v_bitmap, bitmap_pos)){
		bitmap_pos++;
		bit_len++;
	}

	int res = free_page(directory, AFTER_KERNEL, bit_len);
	if(res != 0){
		return 2 | res << 3;
	}

	//Zwalnianie stosu
	bitmap_pos = 0xFFFFFFFF / 0x1000;
	bit_len = 0;
	while(check_bit(directory->v_bitmap, bitmap_pos)){
		bitmap_pos--;
		bit_len++;
	}

	res = free_page(directory, ((bitmap_pos / 1024) << 22) | ((bitmap_pos % 1024) << 12), bit_len);
	if(res != 0){
		return 3 | res << 3;
	}

	for(int i = AFTER_KERNEL >> 22; i < 1024; i++){
		if(directory->directory_v[i] != NULL)
			kfree(directory->directory_v[i]);
	}

	return 0;
}

int unload_program(struct page_directory_struct **directory){
	free_program_pages(*directory);
	free_dir_struct((*directory))

	return 0;
}

int load_program(elf_header *elf, thread_t *thread){
	if(elf == NULL || thread == NULL || thread->directory == NULL)
		return 1;

	struct page_directory_struct *directory = thread->directory;

	unsigned int pages_required = 0;
	int alloc_ret = alloc_tables(directory, 1);
	if(alloc_ret != 0){
		return 2;
	}

	set_page_directory(directory->directory_addr);

	unsigned int data_end = 0x40000000;

	program_header *sections = (program_header *)((char *)elf + elf->program_header_table);
	unsigned int num_sections = elf->program_table_num_entries;
	printf("[LOAD PROGRAM] num sections: %d\n", num_sections);
	for(int i = 0; i < num_sections; i++){
		program_header *t = &sections[i];

		if(t->v_mem_start + t->size_in_memory > data_end)
			data_end = t->v_mem_start + t->size_in_memory;

		unsigned int pages_required = CEIL(t->size_in_memory, 0x1000);
		
		unsigned int get_page_error = 0;
		unsigned int *pages = get_page(pages_required, GET_PAGE_NOMAP, &get_page_error);
		if(get_page_error != 0){
			// printf("[THREAD SETUP] error when getting pages: %d\n", res);
			free_program_pages(directory);
			set_page_directory(kernel_directory_struct.directory_addr);
			return 3;
		}
		int res = map_pages(directory, t->v_mem_start, pages, pages_required, PAGE_RING3);
		if(res != 0){
			// printf("[THREAD SETUP] error when mapping pages: %d\n", res);
			free_program_pages(directory);
			set_page_directory(kernel_directory_struct.directory_addr);
			return 4;
		}

		flush_tlb();

		// kopiowanie kodu
		memcpy_t((char *)elf + t->offset, t->v_mem_start, t->size_in_file);
		printf("Section %d loaded, size: %x\n", i + 1, t->size_in_memory);
	}

	set_page_directory(kernel_directory_struct.directory_addr);

	thread->code = elf->program_entry;

	return 0;
}

int init_stack(thread_t *thread){
	extern unsigned int *memory_bitmap;

	unsigned int stack_addr = 0xFFFFFFFC;
	unsigned int res = 0;
	void *stack_page = get_page(1, GET_PAGE_NOMAP, &res);
	if(res != 0){
		printf("[THREAD SETUP] couldn't get page for stack: %d\n", res);
		clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 1;
	}

	res = map_pages(thread->directory, (void *)stack_addr, stack_page, 1, PAGE_RING3);
	if(res != 0){
		printf("[THREAD SETUP] couldn't map page for stack: %d\n", res);
		clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 2;
	}

	thread->stack = stack_addr;
	return 0;
}

// alokowanie stron dla segmentów
// może jakiś inny system mapowania i alokowania tabel
// gdy jest mapowany program
// można alokować w dowolne miejsce w tabeli

//	funkcja do ładowania programu do pamięci
thread_t *thread_setup(elf_header *elf, unsigned int *error){
	unsigned int program_code = 0;
	unsigned int code_size = 0;

	thread_t *threadt = kmalloc(sizeof(thread_t));
	error_check(threadt, 1)

	unsigned int clone_error = 0;
	struct page_directory_struct *thread_dir = clone_directory(&kernel_directory_struct, &clone_error);
	threadt->directory = thread_dir;
	printf("cloned_dir: %x\n", thread_dir);
	if(clone_error != 0){
		printf("[THREAD SETUP] clone error: %d\n", clone_error);
		kfree(threadt);
		return NULL;
	}

	int res = load_program(elf, threadt);
	if(res != 0){
		printf("[THREAD SETUP] load_program error: %d\n", res);
		free_dir_struct(thread_dir)
		kfree(threadt);

		return NULL;
	}

	res = init_stack(threadt);
	if(res != 0){
		printf("[THREAD SETUP] init_stack error: %d\n", res);
		free_program_pages(threadt->directory);
		free_dir_struct(threadt->directory);
		kfree(threadt);
		
		return NULL;
	}

	threadt->saved_registers.esp = threadt->stack;
	threadt->saved_registers.eip = threadt->code;

	threadt->pid = next_pid++;
	threadt->status = THREAD_STATUS_READY;
	threadt->saved_registers.privl = 3;
	threadt->were_registers_saved = 0;

	if(error != NULL)
		*error = 0;


	return threadt;
}