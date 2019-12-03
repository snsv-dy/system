#include "memory2.h"
#define get_page(v_addr, n_pages) get_page_d(v_addr, n_pages, __LINE__, __FILE__);
// Dzielenie całkowite z zaokrągleniem do góry 
// #define CEIL(a, b) (((a) + (b) - 1) / (b))

void set_bit_in_bitmap(unsigned int *bitmap, unsigned int pos){
	unsigned int j = pos % 32;
	unsigned int i = pos / 32;

	bitmap[i] |= 1 << j;
}

void clear_bit_in_bitmap(unsigned int bitmap[], unsigned int pos){
	unsigned int j = pos % 32;
	unsigned int i = pos / 32;
	
	bitmap[i] &= ~(1 << j);
}

unsigned int check_bit(unsigned int bitmap[], unsigned int pos){
	unsigned int i = pos / 32;
	unsigned int j = pos % 32;

	return bitmap[i] & (1 << j);
}

unsigned int memory_bitmap[PAGE_BITMAP_LENGTH];

unsigned int kernel_directory[1024] __attribute__((aligned(4096)));
unsigned int kernel_page1[1024] __attribute__((aligned(4096)));


unsigned int k_virtual_bitmap[PAGE_BITMAP_LENGTH];

struct page_directory_struct kernel_directory_struct;
struct page_table_list k_page_list;
struct list_node k_node1;

void *kernel_heap;

unsigned int free_memory;
unsigned int total_memory;

unsigned int get_phys_addr(struct page_directory_struct *directory, unsigned int v_addr, unsigned int *success){
	struct list_node *table_node = directory->table_list->head;

	unsigned int pde = v_addr >> 22;
	unsigned int i = 0;
	for(; table_node != NULL && i < pde; i++){
		table_node = table_node->next;
	}

	if(table_node == NULL && i < pde){
		if(success != NULL)
			*success = 0;
		return 0;
	}

	if(success != NULL)
		*success = 1;

	unsigned int pte = (v_addr >> 12) & 0x3FF;
	unsigned int offset = v_addr & 0xFFF;

	return (((unsigned int *)table_node->table_addr)[pte] & 0xFFFFF000) + offset;
}

int map_phys_to_virt(struct page_directory_struct *directory, unsigned int phys_addr, unsigned int v_addr){
	unsigned int pde = v_addr >> 22;
	unsigned int pte = (v_addr >> 12) & 0x3FF;

	struct list_node *ttable = directory->table_list->head;
	for(int i = 0; ttable != NULL && i < pde; i++){
		ttable = ttable->next;
	}

	if(ttable == NULL)
		return 1;

	unsigned int found = 0;
	unsigned int table_phys_addr = get_phys_addr(directory, ttable->table_addr, &found);

	if(!found)
		return 2;

	directory->directory_addr[pde] = table_phys_addr | PAGE_PRESENT_RW;
	ttable->table_addr[pte] = phys_addr | PAGE_PRESENT_RW;
	ttable->free_pages--;

	return 0;
}

int unmap_virt_addr(struct page_directory_struct *directory, unsigned int v_addr){
	unsigned int pde = v_addr >> 22;
	unsigned int pte = (v_addr >> 12) & 0x3FF;

	struct list_node *ttable = directory->table_list->head;
	for(int i = 0; ttable != NULL && i < pde; i++){
		ttable = ttable->next;
	}

	if(ttable == NULL)
		return 1;

	unsigned int found = 0;
	unsigned int table_phys_addr = get_phys_addr(directory, ttable->table_addr, &found);

	if(!found)
		return 2;

	ttable->table_addr[pte] = 0;
	ttable->free_pages++;

	return 0;
}

int alloc_tables(struct page_directory_struct *directory, unsigned int n_tables){
	if(kernel_heap == NULL)
		return 1;

	unsigned int *table_mem = (unsigned int *)heap_malloc_page_aligned(kernel_heap, n_tables * 0x1000);
	if(table_mem == NULL){
		return 2;
	}

	struct list_node *nodes = (struct list_node *)heap_malloc(kernel_heap, sizeof(struct list_node) * n_tables);
	if(nodes == NULL){
		printf("[ALLOC_TABLES] nodes == NULL\n");
		heap_free(table_mem, n_tables);
		return 3;
	}

	for(int i = 0; i < n_tables; i++){
		for(int j = 0; j < 1024; j++){
			table_mem[i * 1024 + j] = 0;
		}

		if(i < n_tables - 1)
			nodes[i].next = &nodes[i + 1];
		else
			nodes[i].next = NULL;

		if(i > 0)
			nodes[i].prev = &nodes[i - 1];
		else
			nodes[i].prev = directory->table_list->tail;

		nodes[i].free_pages = 1024;
		nodes[i].table_addr = (unsigned int)table_mem + 0x1000 * i;
	}

	directory->table_list->tail->next = nodes;
	directory->table_list->tail = &nodes[n_tables - 1];

	directory->table_list->size += n_tables;

	return 0;
}



// void *get_page(unsigned int n_pages, unsigned int *error){
void *get_page_d(unsigned int n_pages, unsigned int *error, int line, char* file){
	
	unsigned int found = 0;
	int j = find_block_in_bitmap(memory_bitmap, PAGE_BITMAP_LENGTH, n_pages, &found);
	
	found = 0;
	unsigned int pte = find_block_in_bitmap(k_virtual_bitmap, PAGE_BITMAP_LENGTH, n_pages, &found);

	if(found){
		if(error != NULL)
			*error = 1;
		printf("no v\n");
		return 0;
	}

	// znajdowanie wolnej strony w katalogu
	int pde = pte / 1024;
	struct list_node *node = kernel_directory_struct.table_list->head;

	unsigned int tables_required = CEIL(n_pages, 1024);
	int next_mul = pte + 1024 - (pte % 1024);
	tables_required += next_mul < (pte + n_pages);

	int i = 0;
	for(i = pde; node != NULL && i < pde + tables_required; i++){
		node = node->next;
	}

	// jeżeli brakuje tabel, to trzeba dodać więcej
	if(kernel_directory_struct.table_list->size < pde + tables_required){
		int res = alloc_tables(&kernel_directory_struct, tables_required);
		if(res != 0){
			printf("[GET PAGE] page not alloced\n");
			if(error != NULL)
				*error = 2;
			return 0;
		}
	}

	unsigned int v_addr = (pte / 1024) << 22 | (pte % 1024) << 12;
	for(int i = 0; i < n_pages; i++, pte++){
		unsigned int t_addr = (pte / 1024) << 22 | (pte % 1024) << 12;

		int map_stat = map_phys_to_virt(&kernel_directory_struct, (j + i) * 0x1000, t_addr);
		if(map_stat != 0){
			if(error != NULL){
				*error = 3 | map_stat << 8;
			}

			return 0;
		}

		set_bit_in_bitmap(memory_bitmap, j + i);
		set_bit_in_bitmap(k_virtual_bitmap, pte);
	}

	free_memory -= n_pages;

	flush_tlb();

	return (void *)v_addr;
}

int free_page(void *v_addr, unsigned int n_pages){
	unsigned int uvaddr = (unsigned int)v_addr;

	unsigned int pde = uvaddr >> 22;
	unsigned int pte = (uvaddr >> 12);

	unsigned int found;
	for(int i = 0; i < n_pages; i++, pte++){
		found = 0;
		unsigned int t_addr = (pde + (pte / 1024)) << 22 | (pte % 1024) << 12;
		unsigned int phys_addr = get_phys_addr(&kernel_directory_struct, (unsigned int)t_addr, &found);
		if(found != 0){
			return 1;
		}

		int map_stat = unmap_virt_addr(&kernel_directory_struct, t_addr);
		if(map_stat != 0){
			return 2;
		}

		clear_bit_in_bitmap(memory_bitmap, pte);
		clear_bit_in_bitmap(k_virtual_bitmap, pte);
	}

	free_memory += n_pages;

	return 0;
}

// void deb(){}

#define KERNEL_HEAP_PAGES 100

int initialize_memory(unsigned int kernel_start, unsigned int kernel_end, multiboot_memory_map *mmap_addr, unsigned int mmap_length){
	for(int i = 0; i < PAGE_BITMAP_LENGTH; i++){
		memory_bitmap[i] = 0xFFFFFFFF;
		k_virtual_bitmap[i] = 0;
	}

	free_memory = 0;
	total_memory = 0;


	unsigned int last_kernel_page = CEIL(kernel_end, 0x1000);
	total_memory += last_kernel_page;

	// mapowanie kernela do katalogu stron
	kernel_directory[0] = (unsigned int)kernel_page1 | 3;
	for(int i = 1; i < 1024; i++){
		kernel_directory[i] = 0;
	}

	for(int i = 0; i < 1024; i++){
		if( i <= last_kernel_page){
			kernel_page1[i] = (0x1000 * i) | 3;
			set_bit_in_bitmap(k_virtual_bitmap, i);
		}else{
			kernel_page1[i] = 0;
		}
	}

	multiboot_memory_map *mp = mmap_addr;
	while(mp < (mmap_addr + mmap_length)){
		if(mp->type == 1){

			// używam tylko niższej części adresu, bo w 32 bitowym systemie, nie będzie można zaadresować większego adresu
			unsigned int start_p = CEIL(mp->addr_low, 4096);
			unsigned int length_p = mp->length_low / 4096;
			if(start_p < last_kernel_page){
				length_p -= last_kernel_page + 1 - start_p;
				start_p = last_kernel_page + 1;
			}
			free_memory += length_p;

			unsigned int map = 0;	
			for(int i = start_p; i < (start_p + length_p); i++){
				clear_bit_in_bitmap(memory_bitmap, i);
				map = 1;
			}

		}
		mp = (multiboot_memory_map *)((unsigned int)mp + mp->size + sizeof(mp->size));
	}

	total_memory += free_memory;

	// inicjowanie listy tabeli stron i struktury katalogu kernela
	kernel_directory_struct.directory_addr = kernel_directory;
	kernel_directory_struct.table_list = &k_page_list;
	kernel_directory_struct.reserve_next = 0;
	k_page_list.head = &k_node1;
	k_page_list.tail = &k_node1;
	k_page_list.size = 1;

	k_node1.next = NULL;
	k_node1.prev = NULL;
	k_node1.table_addr = kernel_page1;
	k_node1.free_pages = 1024 - last_kernel_page;
	

	set_page_directory(kernel_directory);
	enable_paging();

	unsigned int page_got = 0;

	kernel_heap = get_page(KERNEL_HEAP_PAGES, &page_got);
	if(!page_got){
		heap_init(kernel_heap, 0x1000 * KERNEL_HEAP_PAGES);
	}

	alloc_tables(&kernel_directory_struct, 2);
	// printf("[ - DEBG - ] last_kernel_page: %d, kend: %x\n", last_kernel_page, kernel_end);

	// alloc_tables(&kernel_directory_struct, 1020);
	enlarge_heap(kernel_heap, 1024 * 0x1000 + sizeof(struct list_node) * 1024);
	// heap_all_info_c(kernel_heap);

	alloc_tables(&kernel_directory_struct, 1021);

	return 0;
}

unsigned int find_block_in_bitmap(unsigned int *bitmap, unsigned int bit_length, unsigned int size, unsigned int *success){ 
	unsigned int start = 0;
	unsigned int pages_found = 0;
	
	for(unsigned int i = 0; i < bit_length; ){
		
		if(0){
		}else if(bitmap[i] != 0xFFFFFFFF){ // jest przynajmniej jedna wolna strona w tym miejscu
			if(!check_bit(bitmap, start + pages_found)){
				pages_found++;
				
			}else{
				if(pages_found == 0)
					pages_found++;
					
				start += pages_found;
				pages_found = 0;
			}
			
			if(pages_found >= size){
				break;
			}
			
			if((start + pages_found) % 32 == 0){
				i++;
			}
		}else{
			pages_found = 0;
			start += 32;
			i++;
			// start = i * 32;
		}
	}
	
	if(success != NULL)
		*success = !(pages_found >= size);
	
	// printf("[FIND_BLOCK] start: %d, bit_length: %d\n", start, bit_length);
	return start;
}


// debugging realm

// list addresses of tables
// printf("[GET PAGE] tables alloced\n-------\n");
// struct list_node *node = kernel_directory_struct.table_list->head;
// while(node != NULL){
// 	printf("addr: %x\n", node->table_addr);
// 	node = node->next;
// }
// printf("-------\n");


// display content of table
// unsigned int *table2 = kernel_directory_struct.table_list->head->next->table_addr;
// serial_write("\n\ntable2\n");
// for(int i = 0; i < 1024; i++){
// 	serial_x(table2[i]);
// 	serial_write("\n");
// }