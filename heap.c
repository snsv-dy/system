#include "heap.h"

void heap_info(void *heap){
	heap_block *node = (heap_block *)heap;
//	while(node != NULL){

		printf("[%x]\nsize: %d\nprev: %x\nnext: %x\nused: %d\n----------\n", node, node->length, node->prev, node->next, node->used);

//		node = node->next;
//	}
}

void heap_all_info(heap_block *node){
	while(node != NULL){
		heap_info(node);
		node = node->next;
	}
}

void heap_info_c(heap_block *node){
	if(node != NULL)
		printf("[%x] size: %x, prev: %x, next: %x, used: %d\n", node, node->length, node->prev, node->next, node->used);

}

void heap_all_info_c(heap_block *node){
	while(node != NULL){
		printf("[%x] size: %x, prev: %x, next: %x, used: %d\n", node, node->length, node->prev, node->next, node->used);
		node = node->next;
	}
}

void heap_all_info_s(heap_block *node){
	while(node != NULL){
		serial_write("[");
		serial_x(node);
		serial_write("] size: ");
		serial_x(node->length);
		serial_write(", prev: ");
		serial_x(node->prev);
		serial_write(", next: ");
		serial_x(node->next);
		serial_write(", used: ");
		serial_x(node->used);
		serial_write(", deadcode: ");
		serial_x(node->dead);
		serial_write(" ");
		serial_x(node->daed);
		serial_write("\n");
		// printf("[%x] size: %x, prev: %x, next: %x, used: %d\n", node, node->length, node->prev, node->next, node->used);
		node = node->next;
	}
}

int malloc_count = 0;

heap_block *heap_init(void *heap, unsigned int size){
	if(heap == NULL)
		return NULL;
		
	heap_block *heap_t = (heap_block *)heap;
	heap_t->length = size - sizeof(heap_block);
	heap_t->next = NULL;
	heap_t->prev = NULL;
	heap_t->dead = 0xDEADC0DE;
	heap_t->daed = 0xED0CDAED;
	
	heap_t->used = 0;
	
	malloc_count = 0;

	return heap_t;
}

// Łączy sąsiednie bloki
heap_block *merge_heap(heap_block *heap){
	if(heap != NULL && heap->next != NULL && heap->used != 1 && heap->next->used != 1){
		heap->length += heap->next->length + sizeof(heap_block);
		heap->next = heap->next->next;
		if(heap->next != NULL)
			heap->next->prev = heap;
	}

	return heap;
}

void display_heap(heap_block *heap){
	heap_block *t = heap;
	while(t != NULL){
		
		printf("prev:\t%p\n", t->prev);
		printf("next:\t%p\n", t->next);
		printf("length:\t%u\n", t->length);
		printf("used:\t%d\n\n", t->used);
		
		t = t->next;
	}
}
heap_block *split_heap(heap_block *heap, unsigned int size){
	if(heap == NULL || size <= 0 || (size + sizeof(heap_block)) >= heap->length){
		srintf("[SPLIT HEAP] null: %d\n", size);
		return NULL;
	}else{
		// srintf("[SPLIT HEAP] not null: %d\n", size);
	}
	
	void *new_addr = (void *)heap + sizeof(heap_block) + size;
	heap_block *new_block = (heap_block *)new_addr;
	new_block->length = heap->length - sizeof(heap_block) - size;
	new_block->used = 0;
	new_block->prev = heap;
	new_block->next = heap->next;
	if(heap->next != NULL)
		new_block->next->prev = new_block;
	heap->next = new_block;
	
	heap->length = size;


	new_block->dead = 0xDEADC0DE;
	new_block->daed = 0xED0CDAED;
	// srintf("[SPLIT HEAP] Splited: %d\n", size);
	
	return new_block;
}

// extern void *get_page(unsigned int *error, unsigned int n_pages);
extern void *get_page_d(unsigned int n_pages, unsigned int map, unsigned int *error, int line, char* file);
#define get_page(n_pages, map, err) get_page_d(n_pages, map, err, __LINE__, __FILE__);
extern int free_page(void *v_addr, unsigned int n_pages);

void *heap_malloc_page_aligned_d(heap_block *heap, unsigned int size, int line, char *file){
	if(heap == NULL)
		return NULL;

	// printf("[ALIGNED] size: %x\n", size);

	srintf("[MALLOC ALIGNED] malloc aligned: %d, at %s:%d\n", size, file, line);

	if(size % 4 != 0)
		size += 4 - (size % 4);

	heap_block *t = heap, *last_block = heap;


	int block_end = 0;
	unsigned int aligned_start = ((unsigned int)t + sizeof(heap_block)) + 0x1000 - (((unsigned int)t + sizeof(heap_block)) % 0x1000);
	
	int k = 0;
	while(t != NULL){
		k++;

		if(t->used == 1 || t->length < (size + sizeof(heap_block)) ){
			last_block = t;
			t = t->next;
			continue;
		}

		aligned_start = ((unsigned int)t + sizeof(heap_block)) + 0x1000 - (((unsigned int)t + sizeof(heap_block)) % 0x1000);
		
		if((t->length - sizeof(heap_block) - (aligned_start - ((unsigned int)t + sizeof(heap_block)) )) < size){
			last_block = t;
			t = t->next;
			continue;
		}

		break;
	}

	if(t != NULL)
		srintf("[MALLOC ALIGNED] heap block found: mem: %x, len: %d, n: %d\n", t, t->length, k);
	else
		srintf("[MALLOC ALIGNED] heap block found: mem: %x, n: %d\n", t, k);

	if(t == NULL){
		srintf("[MALLOC ALIGNED] enlarging heap\n");
		int res = enlarge_heap(heap, size);
		if(res != 0){
			srintf("[PAGE ALIGNED] Enlagring heap failed\n");
			return NULL;
		}
		// else if(t->next == NULL){
		// 	printf("[PAGE ALIGNED] Enlagring heap failed 2\n");
		// 	return NULL;
		// }
		serial_write("[MALLOC ALIGNED] heap_enlarged\n");
		return heap_malloc_page_aligned_d(heap, size, line, file);
	}

	// printf("[PAGE ALIGNED] block_length: %x\n", t->length);

	split_heap(t, (aligned_start - ((unsigned int)t + sizeof(heap_block))) - sizeof(heap_block) );


	if(t->next == NULL){
		printf("Spliting block for aligned failed\n");
		return NULL;
	}

	t->next->used = 1;

	srintf("[MALLOC ALIGNED] splitting %x:%d, to %d, sizeof heap block: %d\n", t->next, t->next->length, size, sizeof(heap_block));
	split_heap(t->next, size);


	srintf("[MALLOC ALIGNED] malloc aligned at %s:%d succeded\n", file, line);

	return ((void *)t->next + sizeof(heap_block));
}


// sprawdza czy przekazany blok i następny znajdują sie obok siebie w pamięci
// żeby można było je połączyć
int check_heap_continuity(heap_block *heap){
	return heap != NULL && ((unsigned int)heap + sizeof(heap_block) + heap->length == heap->next);
}


// zwiększa stertę wyrównaną do rozmiaru strony (size < 4096, to sterta i tak będzie zwiększona o 4096 ,albo nawet więcej)
// int enlarge_heap(heap_block *heap, unsigned int size){
int enlarge_heap_d(heap_block *heap, unsigned int size, int line, char *file){
	srintf("[ENLARGE HEAP] requesting: %d", size);
	// heap_block *new_block = NULL;
	unsigned int n_pages = CEIL(size, 0x1000) + 1;
	srintf("(%d), at %s:%d\n", n_pages, file, line);

	heap_block *last_block = heap;
	while(last_block->next != NULL)
		last_block = last_block->next;

	// char npages = 0;
	if(last_block->length < sizeof(heap_block)){
		n_pages++;
		srintf("[ENLARGE HEAP] need additional page\n");
		// npages = 1;
	}
	unsigned int got = 0;
	void *new_pages = get_page(n_pages, 1, &got);

	if(got != 0){
		printf("[ENLARGE HEAP] new_pages err: %x\n", got);
		srintf("[ENLARGE HEAP] new_pages err: %x\n", got);
		return 1;
	}
	srintf("[ENLARGE HEAP] get page mem: %x\n", new_pages);	

	// po wywołaniu get_page struktura sterty mogła się zmienić (get_page może wywołać alloc tables, które używa kmalloc 
	// (prawdopodobnie nie możliwe, bo przy inicjalizacji pamięci, alokowane są wszystkie tabele kernela))
	// więc trzeba jeszcze raz znaleść ostatni blok
	last_block = heap;
	while(last_block->next != NULL)
		last_block = last_block->next;

	heap_block *new_block = (heap_block *)new_pages;
	last_block->next = new_block;
	new_block->prev = last_block;
	new_block->length = n_pages * 0x1000;
	new_block->used = 0;

	new_block->dead = 0xDEADC0DE;
	new_block->daed = 0xED0CDAED;

	// new_block->addr = new_block + sizeof(heap_block);

	void *disp_tab = new_block;
	unsigned int disp_len = new_block->length + sizeof(heap_block);

	if(!last_block->used && check_heap_continuity(last_block)){
		merge_heap(last_block);
		disp_tab = last_block;
		disp_len = last_block->length + sizeof(heap_block);
		srintf("[ENLARGE HEAP] merging last block\n");
	}

	srintf("[ENLARGE HEAP] pages after enlarging: \n");
	extern struct page_directory_struct kernel_directory_struct;

	if(size < 0x1000 * 10)
		display_relevant_tables(&kernel_directory_struct, disp_tab, disp_len);


	srintf("[ENLARGE HEAP] %s:%d succeded\n", file, line);

	return 0;
}

void *heap_malloc_d(heap_block *heap, unsigned int size, int line, char *file){
	if(heap == NULL)
		return NULL;

	char d = 0;
	if(size == 0x10){
		d = 1;
	}

	heap_block *t = heap, *last = t;
	if(size % 4 != 0)
		size += 4 - (size % 4);


	while(t != NULL && (t->used || t->length < (size + sizeof(heap_block)))){
		last = t;
		t = t->next;
	}

	if(t == NULL){
		int res = enlarge_heap(heap, size);
		if(res != 0){
			srintf("[HEAP MALLOC] couldn't make heap larger %s:%d\n", file, line);
			return NULL;
		}

		return heap_malloc(heap, size);
	}else{
		// printf("[MALLOC] Heap not malloc\n");
	}
	
	if(t->length > (size + sizeof(heap_block))){
		split_heap(t, size);
		// split_heap(t->next, size);
	}
	
	t->used = 1;

	// heap_all_info_s(heap);
	srintf("[MALLOC] %d at %s:%d\n", size, file, line);
	
	return ((void *)t + sizeof(heap_block));
}

void *heap_calloc(heap_block *heap, unsigned int num, unsigned int size){
	void *t = heap_malloc(heap, size * num);
	
	if(t != NULL){
		for(int i = 0; i < size * num; i++){
			*((char *)t + i) = 0;
		}
	}
	
	return t;
}

void heap_free(heap_block *heap, void *addr){
	if(heap != NULL){
		heap_block *t = (heap_block *)(addr - sizeof(heap_block));
		// while(t != NULL && t->addr != addr){
		// 	t = t->next;
		// }
		
		if(t == NULL || t->used == 0){
			printf("Invalid free\n");
			
		}else{
			t->used = 0;
			// printf("freed: %d\n", t->length);
			
			if(check_heap_continuity(t)){
				merge_heap(t);
				merge_heap(t->prev);
			}
			if(t->next == NULL && t->length > 0x1000){
				if( (t->length + sizeof(heap_block)) % 0x1000 != 0 ){
					// printf("can't free page mismatched block: %x\n", (t->length + sizeof(heap_block)) % 0x1000);
				}
				// if(t % 0x1000 == 0){
				// 	t->prev->next = NULL;
				// }
			}
		}
	}
}

void heap_block_count(heap_block *heap, int full){
	heap_block *block = heap;
	int num_blocks = 0;
	int free_blocks = 0;
	int total_mem = 0;
	int free_mem = 0;
	while(block != NULL){
		num_blocks++;
		free_blocks += block->used == 0;
		total_mem += block->length;
		if(!block->used)
			free_mem += block->length;

		if(full){
			printf("[%d] %x, length: %d, used: %d\n", num_blocks - 1, block, block->length, block->used == 1);
		}

		block = block->next;
	}
	printf("Blocks in heap: %d, free: %d, (%d/%d KiB)\n", num_blocks, free_blocks, free_mem/1024, total_mem/1024);
}


void *heap_realloc(heap_block *heap, void *addr, unsigned int new_size){
	void *t = heap_malloc(heap, new_size);
	
	if(t != NULL){
		heap_block *addr_block = addr - sizeof(heap_block);
		// while(addr_block->addr != addr)
		// 	addr_block = addr_block->next;
			
		if(t != NULL){
			memcpy_t(addr, t, addr_block->length);
		}
		
		heap_free(heap, addr);
	}
	
	return t;
}

unsigned int heap_available_memory(heap_block *heap){
	unsigned int free = 0;

	heap_block *t = heap;
	while(t != NULL){
		if(!t->used){
			free += t->length;
		}
		t = t->next;
	}

	return free;
}

void heap_destroy(void *heap){
	// to be continued
}

extern void *kernel_heap;
heap_block *get_kernel_heap(){
	return (heap_block *)kernel_heap;
}
