#include "thread.h"

extern struct page_directory_struct kernel_directory_struct;
extern unsigned int *some_program;

extern void interrupt_return(
	unsigned int eip,
	unsigned int esp,
	unsigned int privl);

sem_t *killer_sem, *spawner_sem;
void wake_spawner(){
	// sem_post(spawner_sem);
	thread_t *thr_q = get_waiting((spawner_sem->id << 2) | WAIT_SEM);
	if(thr_q != NULL){
		// printf("[SCAL POST] got: %d\n", thr_q->pid);
		thr_q->queue = THREAD;
		queue_thread(thr_q);
	}
}

void wake_killer(){
	thread_t *thr_q = get_waiting((killer_sem->id << 2) | WAIT_SEM);
	if(thr_q != NULL){
		// printf("[SCAL POST] got: %d\n", thr_q->pid);
		thr_q->queue = THREAD;
		queue_thread(thr_q);
	}
}

struct list_info *thread_list = NULL;
struct list_info *thread_queue = NULL;	// Kolejka wątków gotowych do wykonania
struct list_info *sleep_queue = NULL;	// Lista śpiących wątków
struct list_info *death_queue = NULL;
struct list_info *io_queue = NULL;
struct list_info *spawn_queue = NULL;

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

int has_thread_finished(int pid){

	int spawn_getter(spawn_t *a, spawn_t *b){
		return b != NULL && (int)a == b->new_pid;
	}

	// sprawdzanie czy wątek o podanym id jest w kolejce do utworzenia
	if(find_list_by(spawn_queue, pid, spawn_getter) != -1){
		srintf("[HAS THREAD FINISHED] found spawn\n");
		return 0;
	}

	int thread_getter(thread_t *a, thread_t *b){
		return b != NULL && (int)a == b->pid;
	}

	// sprawdzanie czy wątek o podanym id jest w liście wątków
	if(find_list_by(thread_list, pid, thread_getter) != -1){
		srintf("[HAS THREAD FINISHED] found thread\n");
		return 0;
	}

	// szukany wątek prawdopodobnie został zakończony
	return 1;

	// sprawdzanie czy wątek o podanym id jest w kolejce do utworzenia
	// struct lnode *snode = spawn_queue->head;
	// while(snode != NULL){
	// 	spawn_t *sp = snode->value;
	// 	if(sp->new_pid == pid){
	// 		return 0;
	// 	}

	// 	snode = snode->nextl;
	// }

	// // sprawdzanie czy wątek o podanym id jest w liście wątków
	// struct lnode *tnode = thread_list->head;
	// while(snode != NULL){
	// 	thread_t *thread = tnode->value;
	// 	if(thread->pid == pid){
	// 		return 0;
	// 	}

	// 	tnode = tnode->nextl;
	// }

	// // szukany wątek prawdopodobnie został zakończony
	// return 1;
}

// queue_names for thread_info
char *queue_names[] = {
	"ACTIVE",
	"SLEEPING",
	"DYING",
	"IO BLOCKED",
	"WAITING",
	"UNKNOWN"
};

void display_thread_info(){
	struct lnode *node = thread_list->head;
	printf("Running threads:\n");
	while(node != NULL){
		thread_t *th = node->value;
		int queue_id = th->queue;
		if(queue_id < 0 || queue_id > 5)
			queue_id = 5;
		printf("[%c] %s, pid: %d\n", (th->parent == NULL) * 'p' + (th->parent != NULL) * 'c', queue_names[queue_id], th->pid);
		node = node->nextl;
	}
}

void display_sem_info(){
	struct lnode *node = semaphores->head;
	printf("Semaphores: \n");
	while(node != NULL){
		named_sem *nsem = node->value;
		printf("[%d] %s, counter: %d", nsem->params->id, nsem->params->name, nsem->params->counter);
		if(nsem->threads != NULL && nsem->threads->head != NULL){
			printf(", threads: ");
			struct lnode *tnode = nsem->threads->head;
			while(tnode != NULL){
				thread_t *th = tnode->value;
				printf("%d, ", th->pid);
				tnode = tnode->nextl;
			}
		}
		printf("\n");

		node = node->nextl;
	}
}

int named_sem_id = 10000;
int get_next_sem_id(){
	return rand();
}

int sem_init(sem_t *sem, int counter){
	if(sem == NULL)
		return 1;

	sem->name = NULL;
	sem->id = get_next_sem_id();
	sem->counter = counter;

	return 0;
}

named_sem *find_sem(int id){
	struct lnode *node = semaphores->head;
	named_sem *nsem = NULL;
	while(node != NULL){
		nsem = (named_sem *)node->value;
		if(nsem->params->id == id)
			break;

		node = node->nextl;
	}

	return nsem;
}

int sem_unlinker(sem_t *sem){
	if(sem == NULL){
		return 1;
	}

	named_sem *nsem = find_sem(sem->id);
	if(nsem == NULL){
		return 2;
	}

	thread_t *cur_thread = get_current_thread();
	thread_t *res = remove_item(nsem->threads, cur_thread);
	if(res == NULL){
		printf("[UNLINK] res = %d\n", res);
		return 3;
	}

	nsem->num_threads--;
	if(nsem->num_threads == 0){
		printf("[UNLINK] destroying semaphore: %s, num_threads: %d\n", nsem->params->name, nsem->num_threads);
		res = destroy_list(nsem->threads);
		kfree(nsem->threads);
		kfree(nsem->params);
		res = remove_item(semaphores, nsem);
		kfree(nsem);
	}else{
		printf("[UNLINK] %d(%s) threads: ", nsem->params->id, nsem->params->name);	
		struct lnode *nod = semaphores->head;
		while(nod != NULL){
			thread_t *th = nod->value;
			printf("%d ", th->pid);
			nod = nod->nextl;
		}
		printf("\n");
	}

	printf("[UNLINK] sem list(%x): ", semaphores->head);
	struct lnode *nod = semaphores->head;
	while(nod != NULL){
		named_sem *nse = nod->value;
		printf("%d(%s) ", nse->params->id, nse->params->name);
		nod = nod->nextl;
	}
	printf("\n");
}

int named_sem_wait(sem_t *sem){
	if(sem == NULL)
		return 2;

	struct lnode *node = semaphores->head;
	named_sem *nsem = NULL;
	while(node != NULL){
		nsem = (named_sem *)node->value;
		if(nsem->params->id == sem->id)
			break;

		node = node->nextl;
	}

	// printf("[NAMED WAIT] id: %d, counter: %d\n", nsem->params->id, nsem->params->counter);

	if(node == NULL){
		return 3;
	}

	int ret = 0;

	if(nsem->params->counter > 0){
		nsem->params->counter--;
		// printf("[WAIT] switching %s (thread: %d) counter: %d\n", nsem->params->name, get_current_thread()->pid, nsem->params->counter);
	}else{
		ret = 1;
		// printf("[WAIT(%s)] switching %d, counter: %d\n", nsem->params->name, get_current_thread()->pid, nsem->params->counter);
	}

	// return nsem->params->counter <= 0;
	return ret;
}

int named_sem_post(sem_t *sem){
	if(sem == NULL)
		return 2;

	struct lnode *node = semaphores->head;
	named_sem *nsem = NULL;
	while(node != NULL){
		nsem = (named_sem *)node->value;
		if(nsem->params->id == sem->id)
			break;

		node = node->nextl;
	}

	if(node == NULL){
		return 3;
	}

	sem_t *par = nsem->params;
	par->counter++;
	if(par->counter > 0){
		thread_t *thr_q = get_waiting((sem->id << 2) | WAIT_SEM);
		if(thr_q != NULL){
			thr_q->queue = THREAD;
			queue_thread(thr_q);
			par->counter--;
		}
	}

	return 0;
}

#define THREAD_QUEUE_SIZE 10

int sem_opener(sem_t *sem){
	// printf("[OPENER] opening sem %s\n", sem->name);

	char *name = sem->name;

	struct lnode *t = semaphores->head;
	while(t != NULL){
		named_sem *semt = (named_sem *)t->value;
		// printf("[OPENER] name: %s\n", semt->params->name);
		// printf("[OPENER] sem1: %x\n", semt);
		if(!strcmp(semt->params->name, name)){
			break;
		}
		// printf("[OPENER] comped\n");
		t = t->nextl;
	}


	if(t != NULL){
		printf("[OPENER] found t\n");
		named_sem *nsem = (named_sem *)t->value;
		sem->counter = nsem->params->counter;
		sem->id = nsem->params->id;
		// printf("[OPENER] founded list: %x\n", nsem->threads);

		thread_t *cur_thread = get_current_thread();
		if(find_list(nsem->threads, cur_thread) != -1){
			return 2;
		}else{
			int push_stat = push_back(nsem->threads, cur_thread);
			if(push_stat != 0){
				printf("[OPENER] pushing failed: %d\n", push_stat);
				return 3;
			}
			nsem->num_threads++;
			printf("[OPENER] %s num_threads: %d\n", nsem->params->name, nsem->num_threads);
		}

		printf("sem %d\"%s\" thread list( cur thread: %d): ", nsem->params->id, nsem->params->name, cur_thread->pid);
		struct lnode *node = nsem->threads->head;
		while(node != NULL){
			thread_t *th = (thread_t *)node->value;
			if(th == NULL){
				printf("   [OPENER WEIRD] thread in list == NULL\n");
			}else{
				printf("%d ", th->pid);
			}
			node = node->nextl;
		}
		printf("\n");

	}else{
		// printf("[OPENER] creating t\n");
		// dla jeszcze mniejszej framgentacji sterty, może zaalokuj jeden duży blok i porozdzielaj odpowiednio pamięć
		int name_len = strlen(sem->name);
		if(name_len == 0){
			return 4;
		}

		named_sem *nsem = (named_sem *)kmalloc(sizeof(named_sem));
		if(nsem == NULL){
			return 5;
		}

		// nazwa jest ostatnim elementem struktury, więc można alokować od razu pamięć na nazwę, żeby bardziej nie fragmentować sterty
		sem_t *params = (sem_t *)kmalloc(sizeof(sem_t) + name_len + 1);
		if(params == NULL){
			kfree(nsem);
			return 6;
		}
		params->name = (char *)params + sizeof(sem_t);	// uwaga z tą pamięcią

		// char *name = 

		// inicjalizowanie listy wątków należących do semafora
		int mem_size = sizeof(struct list_info) + sizeof(struct lnode) * THREAD_QUEUE_SIZE;
		char *qmem = (char *)kmalloc(mem_size);
		if(qmem == NULL){
			kfree(params);
			kfree(nsem);
			return 7;
		}

		nsem->threads = init_list(qmem, mem_size);
		if(nsem->threads == NULL){
			kfree(params);
			kfree(nsem);
			kfree(qmem);
			return 8;
		}

		int res = push_back(nsem->threads, get_current_thread());
		if(res != 0){
			kfree(params);
			kfree(nsem);
			kfree(qmem);
			return 9 | (res << 4);
		}
		nsem->num_threads = 1;
		// printf("[OPENER] name: %s, %d, %x\n", sem->name, sizeof(sem_t) - sizeof(char *) + name_len + 1, params->name);
		nsem->params = params;
		params->id = get_next_sem_id();
		params->counter = sem->counter;
		sem->id = params->id;
		res = strcpy(sem->name, params->name);
		// for(int i = 0; sem->name[i] != '\0'; i++){
		// 	params->name[i] = sem->name[i];
		// }
		// printf("[OPENER] opened sem params, name: %s, id: %d, counter: %d\n", params->name, params->id, params->counter);
		// printf("[OPENER] thread_list: %x\n", nsem->threads);

		res = push_back(semaphores, nsem);
		if(res != 0){
			kfree(params);
			kfree(nsem);
			kfree(qmem);
			return 10 | (res << 4);
		}
		printf("sem %d\"%s\" thread list( cur thread: %d): ", nsem->params->id, nsem->params->name, get_current_thread()->pid);
		struct lnode *node = nsem->threads->head;
		while(node != NULL){
			thread_t *th = (thread_t *)node->value;
			if(th == NULL){
				printf("   [OPENER WEIRD] thread in list == NULL\n");
			}else{
				printf("%d ", th->pid);
			}
			node = node->nextl;
		}
		printf("\n");
	}

	return 0;
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
			sem_wait(killer_sem);
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

			thread_t *waited = remove_item_by_getter(wait_queue, (t->pid) << 2 | WAIT_PROC);
			if(waited != NULL){
				waited->queue = THREAD;
				queue_thread(waited);
			}

			// printf("thread_queue after killing: ");
			// struct lnode *g = thread_queue->head;
			// while(g != NULL){
			// 	printf("%d -> ", ((thread_t *)g->value)->pid);

			// 	g = g->nextl;
			// }
			// printf("\n");

			remove_item(thread_list, t);
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

			remove_item(thread_list, t);
			kfree(t);
		}

		enable_interrupts();
	}
}

int push_death(thread_t *thread){
	return push_back(death_queue, thread);
}

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

		// srintf("progs: \n");
		for(int i = n; i < 256; i++){
			progs[i] = 0;
			// srintf("%x\n", );
		}

		va_end(lista);
	}
}

int check_prog(int id){
	return id >= 0 && id < 256 && progs[id] != 0;
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
			sem_wait(spawner_sem);
			continue;
		}

		disable_interrupts();
		if(t->type == CREATE_THREAD){
			srintf("[SPAWNER CT] spawning %d\n", t->new_pid);

			// to może przenieść do syscalla lepiej
			if(find_list(death_queue, t->parent) != -1 ||  find_list(thread_list, t->parent) == -1 || t->parent->nchilds >= MAX_THREADS){
				kfree(t);
				enable_interrupts();
				printf("[SPAWNER CT] sorry can't make this thread %d\n", t->new_pid);
				continue;
			}

			thread_t *threadt = kmalloc(sizeof(thread_t));
			if(threadt == NULL){
				printf("[SPAWNER CT] no memory for thread_t\n");
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

			set_page_directory(threadt->directory->directory_phys_addr);

			// wstawianie parametrów do wykonania funkcji wątku na stos
			unsigned int *stack_p = (unsigned int *)threadt->saved_registers.esp;

			t->parent->children[ichild] = threadt;;
			*stack_p = (unsigned int)t->param;
			*(stack_p - 1) = (unsigned int)t->func;
			*(stack_p - 2) = (unsigned int)t->thread_ptr;
			threadt->saved_registers.esp -= 12;

			set_page_directory(kernel_directory_struct.directory_phys_addr);

			push_thread(threadt);
			push_back(thread_list, threadt);
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
				if(error != 0 || th == NULL){
					printf("[SPAWNER CP] thread_error: %d\n", error);

					// uwalnianie wątku czekającego na zakończenie tego
					thread_t *waited = remove_item_by_getter(wait_queue, (t->new_pid) << 2 | WAIT_PROC);
					if(waited != NULL){
						waited->queue = THREAD;
						queue_thread(waited);
					}
				}else{
					queue_thread(th);
					push_back(thread_list, th);
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

int sem_id_getter(void *value){
	return ((sem_t *)value)->id;
}

// stosy dla spawnera i killera
unsigned int kernel_stacks[1024 * 3];

#ifndef THREAD_QUEUE_SIZE
#define THREAD_QUEUE_SIZE 10
#endif

#define SLEEP_SIZE THREAD_QUEUE_SIZE

int init_multitasking(thread_t *thread){
	if(thread == NULL)
		return 1;

	int nqueues = 8;
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
	struct list_info *tl = init_list(queues_mem + queue_mem_size * 7, queue_mem_size);

	if(tq == NULL || sq == NULL || fq == NULL || ioq == NULL || dq == NULL || sems == NULL || tl == NULL){
		printf("NULL here\n");
		kfree(queues_mem);	
		return 3;
	}

	tq->get_num = thread_sleep_getter;
	sq->get_num = thread_sleep_getter;
	wq->get_num = thread_sleep_getter;
	fq->get_num = thread_sleep_getter;
	dq->get_num = thread_sleep_getter;
	tl->get_num = thread_sleep_getter;

	sems->get_num = sem_id_getter;

	if(push_back(tq, (void *)thread) != 0)
		return 4;

	char *sem_mem = (char *)kmalloc(sizeof(sem_t) * 2);
	if(sem_mem == NULL){
		return 5;
	}

	killer_sem = sem_mem;
	spawner_sem = sem_mem + sizeof(sem_t);
	sem_init(killer_sem, 0);
	sem_init(spawner_sem, 0);
	// printf("[INIT M] killer sem params: id: %d, counter: %d, name: %x\n", killer_sem->id, killer_sem->counter, killer_sem->name);
	// printf("[INIT M] spawner sem params: id: %d, counter: %d, name: %x\n", spawner_sem->id, spawner_sem->counter, spawner_sem->name);

	disable_interrupts();
	thread_list = tl;
	thread_queue = tq;
	sleep_queue = sq;
	spawn_queue = fq;
	death_queue = dq;
	io_queue = ioq;
	wait_queue = wq;
	semaphores = sems;
	enable_interrupts();


	thread_t *kthreads = (thread_t *)kmalloc(sizeof(thread_t) * 3);
	thread_t *spawner = kthreads;
	if(kthreads == NULL){
		printf("No memory for kernel threads\n");
		return 6;
	}

	spawner->directory = &kernel_directory_struct;
	spawner->saved_registers.esp = kernel_stacks + 1023;
	spawner->saved_registers.eip = thread_spawner;

	spawner->status = THREAD_STATUS_READY;
	spawner->pid = get_next_pid();
	spawner->saved_registers.privl = 0;
	spawner->were_registers_saved = 0;
	spawner->parent = NULL;
	queue_thread(spawner);

	thread_t *kiler = kthreads + 1;

	kiler->directory = &kernel_directory_struct;
	kiler->saved_registers.esp = kernel_stacks + 1023 + 1024;
	kiler->saved_registers.eip = thread_killer;

	kiler->status = THREAD_STATUS_READY;
	kiler->pid = get_next_pid();
	kiler->saved_registers.privl = 0;
	kiler->were_registers_saved = 0;
	kiler->parent = NULL;
	queue_thread(kiler);


	thread_t *term = kthreads + 2;
	term->directory = &kernel_directory_struct;
	term->saved_registers.esp = kernel_stacks + 1023 + 1024 * 2;
	term->saved_registers.eip = term_thread;

	term->status = THREAD_STATUS_READY;
	term->pid = get_next_pid();
	term->saved_registers.privl = 0;
	term->were_registers_saved = 0;
	term->parent = NULL;
	queue_thread(term);

	push_back(thread_list, spawner);
	push_back(thread_list, kiler);
	push_back(thread_list, term);
	push_back(thread_list, thread);

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

	set_page_directory(next_thread->directory->directory_phys_addr);

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
	set_page_directory(next_thread->directory->directory_phys_addr);
	return &(next_thread->saved_registers);
}

void thread_start(thread_t *thread){
	if(thread != NULL){
		thread->status = THREAD_STATUS_ACTIVE;
		set_page_directory(thread->directory->directory_phys_addr);
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
	int err = 0;
	t->directory_phys_addr = get_phys_addr(src, dir_p, &err);
	if(err == 0){
		srintf("[CLONE DIR] did not get directory phys addr: %d, dir: %x\n", err, t->directory_addr);
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
int stophere(){return 2;}
int load_program(elf_header *elf, thread_t *thread){
	if(elf == NULL || thread == NULL || thread->directory == NULL)
		return 1;
	srintf("[LOAD PROGRAM] loading program %x\n", elf);
	struct page_directory_struct *directory = thread->directory;

	unsigned int pages_required = 0;
	int alloc_ret = alloc_tables_at(directory, AFTER_KERNEL >> 22, 1);
	if(alloc_ret != 0){
		return 2;
	}
	// srintf("[LOAD PROGRAM] tables alloced, directory[256]: %x, v[256]: %x\n", directory->directory_addr[256], directory->directory_v[256]);
	srintf("[LOAD PROGRAM] changing directory: %x\ndirectory check: \n", directory->directory_addr);
	// for(int i = 0; i < 1024; i++)
	// srintf()
	set_page_directory(directory->directory_phys_addr);

	srintf("[LOAD PROGRAM] directory changed\n");

	unsigned int data_end = 0x40000000;

	program_header *sections = (program_header *)((char *)elf + elf->program_header_table);
	unsigned int num_sections = elf->program_table_num_entries;
	srintf("[LOAD PROGRAM] num sections: %d\n", num_sections);
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
			set_page_directory(kernel_directory_struct.directory_phys_addr);
			return 3;
		}
		int res = map_pages(directory, t->v_mem_start, pages, pages_required, PAGE_RING3);
		if(res != 0){
			printf("[THREAD SETUP] error when mapping pages: %d\n", res);
			free_program_pages(directory);
			set_page_directory(kernel_directory_struct.directory_phys_addr);
			return 4;
		}

		flush_tlb();

		// kopiowanie kodu
		memcpy_t((char *)elf + t->offset, t->v_mem_start, t->size_in_file);
		// printf("Section %d loaded, size: %x\n", i + 1, t->size_in_memory);
	}
	srintf("[LOAD PROGRAM] sections copied\n");

	set_page_directory(kernel_directory_struct.directory_phys_addr);

	// thread->code = elf->program_entry;
	thread->saved_registers.eip = elf->program_entry;

	return 0;
}

extern unsigned int *memory_bitmap;

// Funkcja alokująca pierwszą stronę stosu dla wątku, 
// thread pos określa jak daleko licząc od końca pamięci, zaczyna się stos i jednocześnie
// który to jest wątek w katalogu stron, ponieważ wątki w ramach tego samego procesu, współdzielą pamięć
int init_stack(thread_t *thread, int thread_pos){

	unsigned int stack_addr = 0xFFFFFFFC - (thread_pos * THREAD_STACK_SIZE);
	unsigned int res = 0;
	void *stack_page = get_page(1, GET_PAGE_NOMAP, &res);
	if(res != 0){
		printf("[STACK INIT] couldn't get page for stack: %d\n", res);
		// clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 1;
	}
	srintf("[INIT STACK] page got\n");

	res = map_pages(thread->directory, (void *)stack_addr, stack_page, 1, PAGE_RING3);
	if(res != 0){
		printf("[STACK INIT] couldn't map page for stack: %d\n", res);
		clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
		return 2;
	}
	srintf("[INIT STACK] page mapped\n");

	thread->stack_pages = 1;
	thread->stack_start = stack_addr;
	thread->max_stack_pages = THREAD_STACK_SIZE / 0x1000; // to powinno być jakąś stałą wartością nie należącą do struktury thread_t

	// thread->stack = stack_addr;
	thread->saved_registers.esp = stack_addr;

	srintf("[INIT STACK] stack inited\n");
	return 0;
}


// Funkcja rozszerzająca stos, wywoływana przez page fault
int enlarge_stack(thread_t *thread, unsigned int v_addr){
	if(thread == NULL || v_addr == NULL){
		return 1;
	}


	// printf("thread: %d\n", thread->pid);
	unsigned int stack_start = thread->stack_start / 0x1000;
	unsigned int page = v_addr / 0x1000;
	unsigned int relative_page = stack_start - page;
	unsigned int pages_required = relative_page - (thread->stack_pages - 1);
	// printf("accessing: %d, stack_start: %d\n", page, stack_start);
	// printf("pages: %d\n", thread->stack_pages);

	// printf("trying to access: %d/%d, (%d)\n", relative_page, thread->max_stack_pages, thread->stack_pages);
	// printf("required_pages: %d\n", pages_required);

	if(relative_page < thread->max_stack_pages){
		int res = 0;
		void *stack_page = get_page(pages_required, GET_PAGE_NOMAP, &res);
		if(res != 0){
			srintf("[ENLARGE STACK] couldn't get page for stack: %d\n", res);
			// clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
			return 3;
		}

		res = map_pages(thread->directory, (void *)v_addr, stack_page, pages_required, PAGE_RING3);
		if(res != 0){
			srintf("[ENLARGE STACK] couldn't map page for stack: %d\n", res);
			clear_bit_in_bitmap(memory_bitmap, (unsigned int)stack_page / 0x1000);
			return 4;
		}
		thread->stack_pages += pages_required;
	}else{
		return 2;
	}
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

	srintf("[THREAD SETUP] elf: %x, pid: %d, thread: %x\n", elf, pid, threadt);

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
	srintf("[THREAD SETUP] program_loaded\n");
	if(res != 0){
		printf("[THREAD SETUP] load_program error: %d\n", res);
		free_dir_struct(thread_dir)
		kfree(threadt);

		return NULL;
	}

	res = init_stack(threadt, 0);
	if(res != 0){
		srintf("[THREAD SETUP] init_stack error: %d\n", res);
		free_program_pages(threadt->directory);
		free_dir_struct(threadt->directory);
		kfree(threadt);
		
		return NULL;
	}
	srintf("[THREAD SETUP] stack inited\n");
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

	srintf("[THREAD SETUP] thread loaded\n");

	return threadt;
}