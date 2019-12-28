#include "thread.h"

extern struct page_directory_struct kernel_directory_struct;
extern unsigned int *some_program;

extern void interrupt_return(
	unsigned int eip,
	unsigned int esp,
	unsigned int privl);


struct list_info *thread_queue = NULL;	// Kolejka wątków gotowych do wykonania
struct list_info *sleep_queue = NULL;	// Lista śpiących wątków
struct list_info *death_queue = NULL;
struct list_info *io_queue = NULL;

struct list_info *wait_queue = NULL;	// Lista wątków czekających na semaforach
struct list_info *semaphores = NULL;

thread_t *get_waiting(int id){ return remove_item_by_getter(wait_queue, id); }
int push_wait(thread_t *t) { return push_back(wait_queue, t); }

thread_t *pop_io(){ return pop_front(io_queue); }
int push_io(thread_t *t) { return push_back(io_queue, t); }

unsigned int switching_go = 0;

int next_pid = 1;
int get_next_pid(){
	return next_pid++;
}

#define free_dir_struct(dir) \
	if(dir != NULL) {\
		if(dir->directory_addr != NULL) \
			kfree(dir->directory_addr); \
		if(dir->directory_v != NULL) \
			kfree(dir->directory_v); \
		kfree(dir); \
	}
#define pop_thread() (thread_t *)pop_front(thread_queue)
#define push_thread(x) push_back(thread_queue, x)
#define to_thread(x) (thread_t *)(x)

void queue_thread(thread_t *t){
	push_thread(t);
}

// używane w killerze do usuwania wątku z jakiejkolwiek kolejki
int remove_thread_from_queues(thread_t *t){
	if(t == NULL)
		return 1;

	switch(t->queue){
		case THREAD: { return remove_item(thread_queue, t); } 
		case SLEEP: { 
			t->pid = 0xFFFFFFFF;
			return NULL;
		} 
		case IO: { return remove_item(io_queue, t); } 
		case DEATH: { return remove_item(death_queue, t); }
		case WAIT: { return remove_item(wait_queue, t); } 
		default: return 2;
	}
}

void thread_killer(){
	thread_t *t;
	while(1){
		if(death_queue == NULL){
			continue;
		}
		t = pop_front(death_queue);
		if(t == NULL){
			// tu powinien być yeld, ale narazie założyłem, że syscalle są tylko z poziomu użytkownika
			continue;
		}
		disable_interrupts();

		printf("[KILLER] killing: %d\n", t->pid);

		int threadi = 0;
		if(t->parent != NULL){
			// usuwanie wątku utworzonego przez funkcję create_thread
			for(; threadi < MAX_THREADS; threadi++){
				if(t->parent->children[threadi] == t){
					t->parent->children[threadi] = 0;
					break;
				}
			}

			t->parent->nchilds--;

			// usuwanie stosu
			unsigned int stack_beg = 0xFFFFFFFC - ((threadi + 1) * THREAD_STACK_SIZE);
			unsigned int stack_page = stack_beg / 0x1000;

			unsigned int *directory = t->directory->directory_v;

			unsigned int pde = stack_beg >> 22;
			unsigned int pte = (stack_beg >> 12) & 0x3FF;

			unsigned int *table = directory[pde];

			for(int i = pte, j = 0; i >= 0; i--, j++){
				if(table[i] == 0){
					break;
				}

				unsigned int spage = stack_beg - (j * 0x1000);

				int res = free_page(t->directory, spage, 1);
				if(res != 0){
					printf("[KILLA] freeing page error: %d\n", res);
				}
			}

			// printf("thread_queue after killing: ");
			// struct lnode *g = thread_queue->head;
			// while(g != NULL){
			// 	printf("%d -> ", ((thread_t *)g->value)->pid);

			// 	g = g->nextl;
			// }
			// printf("\n");

			kfree(t);
		}else{  // Usuwanie wątku utworzonego przez funkcję create_process, lub utworzonego przy starcie systemu
			// usuwanie wątków utworzonych przez ten wątek
			if(t->children != NULL && t->nchilds > 0){
				for(int i = 0, j = 0; i < t->children_capacity && j < t->nchilds; i++){
					if(t->children[i] != NULL){
						thread_t *nt = t->children[i]; // nested thread
						thread_t *returned = remove_thread_from_queues(nt);
						if(nt->queue != SLEEP && (nt != returned || returned == NULL)){
							printf("[KILLER] removing nested thread from queue failed: %d\n", nt->pid);
						}else{
							// printf("[KILLER] freeing child: %d\n", nt->pid);
							kfree(nt);
						}
						j++;
					}
				}
				kfree(t->children);
			}
			

			int res = free_whole_program(t->directory);
			if(res != 0){
				printf("[KILLER] freeing whole program failed\n");
			}

			thread_t *waited = remove_item_by_getter(wait_queue, (t->pid) << 2 | WAIT_PROC);
			if(waited != NULL){
				waited->queue = THREAD;
				queue_thread(waited);
			}

			kfree(t);
		}

		enable_interrupts();
	}
}

int push_death(thread_t *thread){
	return push_back(death_queue, thread);
}

struct list_info *spawn_queue = NULL;

int clear_stack(thread_t *thread){
	if(thread == NULL || thread->directory == NULL){
		printf("[CLEAR STACK] thread == NULL || thread->directory == NULL\n");
		return 1;
	}

	unsigned int *arrp = thread->directory->directory_addr;
	unsigned int *arrv = thread->directory->directory_v;
	for(int i = 1023; arrp[i] != 0 && i >= 256; i--){
		arrp[i] = 0;
		arrv[i] = 0;
	}

	return 0;
}


unsigned int *progs[256] = {0};
void set_progs(int n, ...){
	if(n > 0){
		va_list lista;
		va_start(lista, n);

		for(int i = 0; i < n; i++){
			unsigned int *t = va_arg(lista, unsigned int *);
			progs[i] = t;
		}

		va_end(lista);
	}
}

// wątek tworzący nowe wątki
void thread_spawner(){
	spawn_t *t;
	do{
		if(spawn_queue == NULL){
			continue;
		}
		t = pop_front(spawn_queue);
		if(t == NULL){
			// tu powinien być yeld, ale narazie założyłem, że syscalle są tylko z poziomu użytkownika
			// i nie mam ochoty teraz tego zmieniać
			continue;
		}

		disable_interrupts();
		if(t->type == CREATE_THREAD){
			// printf("[SPAWNER CT] spawning %d\n", t->new_pid);
			
			// to może przenieść do syscalla lepiej
			if(t->parent->nchilds >= MAX_THREADS){
				kfree(t);
				enable_interrupts();
				continue;
			}

			thread_t *threadt = kmalloc(sizeof(thread_t));
			if(threadt == NULL){
				printf("[SPAWNER] no memory for thread_t\n");
				kfree(t);
				enable_interrupts();
				continue;
			}


			thread_t *parent = t->parent;
			while(parent->parent != NULL){
				parent = parent->parent;
			}

			threadt->parent = parent;

			threadt->directory = threadt->parent->directory;

			if(t->parent->children == NULL){
				// alokowanie pamięci na tablicę wątków
				thread_t **ch_arr = (thread_t **)kcalloc(MAX_THREADS, sizeof(thread_t *));
				if(ch_arr == NULL){
					printf("[SPAWNER CT] ch_arr == NULL\n");
					kfree(threadt);
					kfree(t);
					enable_interrupts();
					break;
				}
				t->parent->children = ch_arr;
			}

			int ichild = 0;
			while(t->parent->children[ichild] != 0){
				ichild++;
			}

			unsigned int stack_start = 0xFFFFFFFC - ((ichild + 1) * THREAD_STACK_SIZE);
			int res = init_stack(threadt, ichild + 1);
			if(res != 0){
				printf("[SPAWNER CT] stack init failed: %d\n", res);
				kfree(threadt);
				kfree(t);
				enable_interrupts();
				break;
			}

			threadt->saved_registers.eip = t->eip;
			threadt->pid = t->new_pid;
			threadt->status = THREAD_STATUS_READY;
			threadt->saved_registers.privl = 3;
			threadt->were_registers_saved = 0;
			threadt->queue = THREAD;
			
			printf("[SPAWNER CT] setting stack params\n");

			set_page_directory(threadt->directory->directory_addr);

			// wstawianie parametrów do wykonania funkcji wątku na stos
			unsigned int *stack_p = (unsigned int *)threadt->saved_registers.esp;

			t->parent->children[ichild] = threadt;;
			*stack_p = (unsigned int)t->param;
			*(stack_p - 1) = (unsigned int)t->func;
			*(stack_p - 2) = (unsigned int)t->thread_ptr;
			threadt->saved_registers.esp -= 12;

			set_page_directory(kernel_directory_struct.directory_addr);

			push_thread(threadt);
			kfree(t);
		}else if(t->type == CREATE_PROCESS){

			int prog_id = (int)t->param;
			if(prog_id > 255 || prog_id < 0){
				printf("[SPAWNER] prog_id exceeding numbers of programs in system: %d\n", prog_id);
				kfree(t);
				enable_interrupts();
				continue;
			}
			if(progs[prog_id] != NULL){
				elf_header *elf = progs[prog_id];
				printf("[SPAWNER CP] loading %x, %d\n", elf, progs[prog_id]);
				int error = 0;
				thread_t *th = thread_setup(elf, t->new_pid, &error);
				if(error != 0){
					printf("[SPAWNER CP] thread_error: %d\n", error);	
				}else{
					queue_thread(th);
				}
				
			}else{
				printf("[SPAWNER CP] progs[%d] == NULL\n", prog_id);
			}
			kfree(t);
		}

		printf("thread_queue: ");
		struct lnode *t = thread_queue->head;
		while(t != NULL){
			printf("%d -> ", ((thread_t *)t->value)->pid);
			t = t->nextl;
		}
		printf("\n");
		enable_interrupts();
	}while(1);
}

int push_spawn(spawn_t *t){
	int res = push_back(spawn_queue, t);
	return res;
}

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

void put_thread_to_sleep(thread_t *thread, int how_long){
	if(thread != NULL && sleep_queue != NULL){
		thread->sleep_pos = how_long;
		queue_num(sleep_queue, thread);
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
	if(thread_queue == NULL){
		printf("[TIMER] Thread queue == NULL\n");
	}

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
			if(t->pid == 0xFFFFFFFF){
				kfree(t);
			}else{
				t->status = THREAD_STATUS_READY;
				t->queue = THREAD;
				push_back(thread_queue, t);
			}
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

	return cur_state;
}

// stosy dla spawnera i killera
unsigned int kernel_stacks[1024 * 2];

#define THREAD_QUEUE_SIZE 10
#define SLEEP_SIZE THREAD_QUEUE_SIZE

int init_multitasking(thread_t *thread){
	if(thread == NULL)
		return 1;

	int nqueues = 7;
	unsigned int queue_mem_size = (sizeof(struct list_info) + sizeof(struct lnode) * THREAD_QUEUE_SIZE);
	char *queues_mem = (char *)kmalloc( queue_mem_size * nqueues);
	if(queues_mem == NULL)
		return 2;

	struct list_info *tq = init_list(queues_mem, queue_mem_size);
	struct list_info *sq = init_list(queues_mem + queue_mem_size, queue_mem_size);
	struct list_info *fq = init_list(queues_mem + queue_mem_size * 2, queue_mem_size);
	struct list_info *dq = init_list(queues_mem + queue_mem_size * 3, queue_mem_size);
	struct list_info *ioq = init_list(queues_mem + queue_mem_size * 4, queue_mem_size);
	struct list_info *wq = init_list(queues_mem + queue_mem_size * 5, queue_mem_size);

	struct list_info *sems = init_list(queues_mem + queue_mem_size * 6, queue_mem_size);
	if(tq == NULL || sq == NULL || fq == NULL || ioq == NULL || dq == NULL || sems == NULL){
		printf("NULL here\n");
		kfree(queues_mem);	
		return 3;
	}

	tq->get_num = thread_sleep_getter;
	sq->get_num = thread_sleep_getter;
	wq->get_num = thread_sleep_getter;
	fq->get_num = thread_sleep_getter;
	dq->get_num = thread_sleep_getter;

	if(push_back(tq, (void *)thread) != 0)
		return 4;

	disable_interrupts();
	thread_queue = tq;
	sleep_queue = sq;
	spawn_queue = fq;
	death_queue = dq;
	io_queue = ioq;
	wait_queue = wq;
	semaphores = sems;
	enable_interrupts();


	thread_t *spawner = (thread_t *)kmalloc(sizeof(thread_t));
	if(spawner == NULL){
		printf("No memory for spawner\n");
		return;
	}

	spawner->directory = &kernel_directory_struct;
	spawner->saved_registers.esp = kernel_stacks + 1023;
	spawner->saved_registers.eip = thread_spawner;

	spawner->status = THREAD_STATUS_READY;
	spawner->pid = get_next_pid();
	spawner->saved_registers.privl = 0;
	spawner->were_registers_saved = 0;
	queue_thread(spawner);

	thread_t *kiler = (thread_t *)kmalloc(sizeof(thread_t));
	if(kiler == NULL){
		printf("No memory for kiler\n");
		return;
	}

	kiler->directory = &kernel_directory_struct;
	kiler->saved_registers.esp = kernel_stacks + 1023 + 1024;
	kiler->saved_registers.eip = thread_killer;

	kiler->status = THREAD_STATUS_READY;
	kiler->pid = get_next_pid();
	kiler->saved_registers.privl = 0;
	kiler->were_registers_saved = 0;
	queue_thread(kiler);
	printf("kernel_pids: killa: %d, spawner: %d", kiler->pid, spawner->pid);

	return 0;
}

proc_state *sleep_task(unsigned int saved_eip, unsigned int sqved_esp, unsigned int num_ticks){
	if(thread_queue == NULL)
		return NULL;

	thread_t *current_thread = pop_thread();
	current_thread->status = THREAD_STATUS_WAITING;
	current_thread->queue = SLEEP;

	put_thread_to_sleep(current_thread, num_ticks);

	thread_t *next_thread = (thread_t *)thread_queue->head->value;
	if(next_thread == NULL)
		return NULL;
	

	next_thread->status = THREAD_STATUS_ACTIVE;

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
	return &(next_thread->saved_registers);
}

void thread_start(thread_t *thread){
	if(thread != NULL){
		thread->status = THREAD_STATUS_ACTIVE;
		set_page_directory(thread->directory->directory_addr);
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

// to kopiuje płytko
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



#define AFTER_KERNEL 0x40000000

// zwania przekazywany parametr directory
int free_whole_program(struct page_directory_struct *directory){
	if(directory == NULL)
		return 1;

	int bitmap_pos = AFTER_KERNEL / 0x1000;
	int bit_len = 0;

	for(int pde = AFTER_KERNEL >> 22; pde < 1024; pde++){
		unsigned int *table = directory->directory_v[pde];
		if(table != NULL){
			for(int pte = 0; pte < 1024; pte++){
				if(table[pte] != 0){
					int res = free_page(directory, (pde << 22) | (pte << 12), 1);
					if(res != 0){
						printf("[FR WH PROG] freeing page failed: %d, %d, %d\n", res, pde, pte);
					}	
				}
			}
		}
	}

	for(int i = AFTER_KERNEL / (1024 * 4096); i < 1024; i++){
		if(directory->directory_v[i] != 0){
			kfree(directory->directory_v[i]);
		}
	}

	kfree(directory->v_bitmap);
	kfree(directory->directory_addr);
	kfree(directory->directory_v);
	kfree(directory);

	return 0;
}

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
	bitmap_pos = 0x100000000 / 0x1000 - 1;
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
	// printf("[LOAD PROGRAM] num sections: %d\n", num_sections);
	for(int i = 0; i < num_sections; i++){
		program_header *t = &sections[i];

		if(t->v_mem_start + t->size_in_memory > data_end)
			data_end = t->v_mem_start + t->size_in_memory;

		unsigned int pages_required = CEIL(t->size_in_memory, 0x1000);
		
		// printf("[LOAD PROGRAM] pages requires for section %d, %d\n", i + 1, pages_required);

		unsigned int get_page_error = 0;
		unsigned int *pages = get_page(pages_required, GET_PAGE_NOMAP, &get_page_error);
		if(get_page_error != 0){
			printf("[THREAD SETUP] error when getting pages: %d\n", get_page_error);
			free_program_pages(directory);
			set_page_directory(kernel_directory_struct.directory_addr);
			return 3;
		}
		int res = map_pages(directory, t->v_mem_start, pages, pages_required, PAGE_RING3);
		if(res != 0){
			printf("[THREAD SETUP] error when mapping pages: %d\n", res);
			free_program_pages(directory);
			set_page_directory(kernel_directory_struct.directory_addr);
			return 4;
		}

		flush_tlb();

		// kopiowanie kodu
		memcpy_t((char *)elf + t->offset, t->v_mem_start, t->size_in_file);
		// printf("Section %d loaded, size: %x\n", i + 1, t->size_in_memory);
	}

	set_page_directory(kernel_directory_struct.directory_addr);

	// thread->code = elf->program_entry;
	thread->saved_registers.eip = elf->program_entry;

	return 0;
}

int init_stack(thread_t *thread, int thread_pos){
	extern unsigned int *memory_bitmap;

	unsigned int stack_addr = 0xFFFFFFFC - (thread_pos * THREAD_STACK_SIZE);
	unsigned int res = 0;
	void *stack_page = get_page(1, GET_PAGE_NOMAP, &res);
	if(res != 0){
		printf("[STACK INIT] couldn't get page for stack: %d\n", res);
		clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 1;
	}

	res = map_pages(thread->directory, (void *)stack_addr, stack_page, 1, PAGE_RING3);
	if(res != 0){
		printf("[STACK INIT] couldn't map page for stack: %d\n", res);
		clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 2;
	}

	// thread->stack = stack_addr;
	thread->saved_registers.esp = stack_addr;
	return 0;
}

// alokowanie stron dla segmentów
// może jakiś inny system mapowania i alokowania tabel
// gdy jest mapowany program
// można alokować w dowolne miejsce w tabeli

//	funkcja do ładowania programu do pamięci
thread_t *thread_setup(elf_header *elf, int pid, unsigned int *error){
	unsigned int program_code = 0;
	unsigned int code_size = 0;

	thread_t *threadt = kmalloc(sizeof(thread_t));
	error_check(threadt, 1)

	unsigned int clone_error = 0;
	struct page_directory_struct *thread_dir = clone_directory(&kernel_directory_struct, &clone_error);
	threadt->directory = thread_dir;
	// printf("cloned_dir: %x\n", thread_dir);
	if(clone_error != 0){
		printf("[THREAD SETUP] clone error: %d\n", clone_error);
		kfree(threadt);
		return NULL;
	}

	int res = load_program(elf, threadt);
	// printf("[SETUP] program_loaded\n");
	if(res != 0){
		printf("[THREAD SETUP] load_program error: %d\n", res);
		free_dir_struct(thread_dir)
		kfree(threadt);

		return NULL;
	}

	res = init_stack(threadt, 0);
	if(res != 0){
		printf("[THREAD SETUP] init_stack error: %d\n", res);
		free_program_pages(threadt->directory);
		free_dir_struct(threadt->directory);
		kfree(threadt);
		
		return NULL;
	}

	// threadt->saved_registers.esp = threadt->stack;
	// threadt->saved_registers.eip = threadt->code;
	threadt->pid = get_next_pid();
	if(pid != -1)
		threadt->pid = pid;

	threadt->status = THREAD_STATUS_READY;
	threadt->saved_registers.privl = 3;
	threadt->were_registers_saved = 0;
	threadt->children = NULL;
	threadt->nchilds = 0;
	threadt->children_capacity = MAX_THREADS;
	threadt->parent = NULL;
	threadt->queue = THREAD;

	if(error != NULL)
		*error = 0;


	return threadt;
}