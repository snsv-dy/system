#include "memory2.h"
// Dzielenie całkowite z zaokrągleniem do góry 
// #define CEIL(a, b) (((a) + (b) - 1) / (b))

void set_bit_in_bitmap(unsigned int *bitmap, unsigned int pos){
	unsigned int j = pos % 32;
	unsigned int i = pos / 32;
	// srintf("Setting bitmap\n");
	bitmap[i] |= 1 << j;
	// srintf("Bitmap set\n");
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
unsigned int *kernel_directory_v[1024];		// wirtualne adresy tabel

unsigned int kernel_page1[1024] __attribute__((aligned(4096)));


unsigned int k_virtual_bitmap[PAGE_BITMAP_LENGTH];

struct page_directory_struct kernel_directory_struct;

void *kernel_heap;

unsigned int free_memory;
unsigned int total_memory;

unsigned int get_phys_addr(struct page_directory_struct *directory, unsigned int v_addr, unsigned int *success){

	unsigned int pde = v_addr >> 22;
	unsigned int *table = directory->directory_v[pde];

	if(table == NULL){
		if(success != NULL)
			*success = 0;
		return 0;
	}

	if(success != NULL)
		*success = 1;

	unsigned int pte = (v_addr >> 12) & 0x3FF;
	unsigned int offset = v_addr & 0xFFF;

	return (table[pte] & 0xFFFFF000) | offset;
}

int map_phys_to_virt(struct page_directory_struct *directory, unsigned int phys_addr, unsigned int v_addr, unsigned int flags){
	unsigned int pde = v_addr >> 22;
	unsigned int pte = (v_addr >> 12) & 0x3FF;


	unsigned int *table = directory->directory_v[pde];
	if(v_addr >= 0x40000000)
		srintf("[MAP PHYS TO VIRT] table: %x\n", table);

	if(table == NULL){

		printf("MAP PHYS RETURN 1 %d\n", pde);
		return 1;
	}

	unsigned int found = 0;
	// srintf("[MAP PHYS TO VIRT] getting phys_Addr\n");
	// srintf("[MAP PHYS TO VIRT] phys_Addr got: %x\n", table_phys_addr);
	unsigned int table_phys_addr = get_phys_addr(&kernel_directory_struct, table, &found);

	if(v_addr >= 0x40000000)
		srintf("[MAP PHYS TO VIRT] table_phys_addr: %x\n", table_phys_addr);

	if(!found){
		srintf("[MAP PHYS TO VIRT] phys for table: %x(%d) not found\n", table, pde);
		return 2;
	}

	flags &= 0xFFF;

	directory->directory_addr[pde] = table_phys_addr | PAGE_PRESENT_RW | flags;
	table[pte] = phys_addr | PAGE_PRESENT_RW | flags;

	if(v_addr >= 0x40000000)
		srintf("[MAP PHYS TO VIRT] addr mapped\n");

	return 0;
}

int unmap_virt_addr(struct page_directory_struct *directory, unsigned int v_addr, unsigned int flags){
	unsigned int pde = v_addr >> 22;
	unsigned int pte = (v_addr >> 12) & 0x3FF;

	unsigned int *table = directory->directory_v[pde];

	if(table == NULL)
		return 1;

	unsigned int found = 0;
	unsigned int table_phys_addr = get_phys_addr(&kernel_directory_struct, table, &found);

	if(!found)
		return 2;

	// flags &= 0xFFF;

	// directory->directory_addr[pde] = table_phys_addr | PAGE_PRESENT_RW | flags;
	table[pte] = 0;

	return 0;
}

int alloc_tables_at_d(struct page_directory_struct *directory, int start, unsigned int n_tables, int line, char *file){
	if(kernel_heap == NULL || start == -1 || n_tables == 0)
		return 1;

	for(int i = start; i < start + n_tables; i++){
		if(directory->directory_v[i] != NULL){
			return 2;
		}
	}

	unsigned int *table_mem = (unsigned int *)heap_malloc_page_aligned(kernel_heap, n_tables * 0x1000);
	if(table_mem == NULL){
		return 3;
	}

	for(int i = 0; i < n_tables; i++){
		for(int j = 0; j < 1024; j++){
			table_mem[i * 1024 + j] = 0;
		}
	}

	for(int i = 0; i < n_tables; i++){
		directory->directory_v[start + i] = &table_mem[1024 * i];
		// int err = 0;
		// unsigned int table_phys = get_phys_addr(directory, &table_mem[1024 * i], &err);
		// if(err != 0){
		// 	srintf("[ALLOC TABLES AT] get phys_error: %d, at: %x\n", err, &table_mem[1024 * i]);
		// }else{
		// 	directory->directory_addr[start + i] = table_phys;
		// }
	}

	directory->num_tables += n_tables;

	srintf("[ALLOC TABLES AT] start: %d, ntables: %d, at %s:%d\n", start, n_tables, file, line);

	return 0;
}
int alloc_tables_d(struct page_directory_struct *directory, unsigned int n_tables, int line, char *file){
	srintf("[ALLOC TABLES] n_tables: %d, at %s:%d ", n_tables, file, line);
	if(kernel_heap == NULL){
		srintf(" failed: %d\n", 1);
		return 1;
	}

	unsigned int tables_end = 0;
	while(tables_end < 1024 && directory->directory_v[tables_end] != 0) tables_end++;

	if(tables_end + n_tables >= 1024)
		n_tables = 1024 - tables_end;
	// printf("[ALLOC TABLES] n_tables: %d, tables_end: %d\n", n_tables, tables_end);
	srintf("\n\t");
	unsigned int *table_mem = (unsigned int *)heap_malloc_page_aligned(kernel_heap, n_tables * 0x1000);
	if(table_mem == NULL){
		srintf(" failed: %d\n", 2);
		return 2;
	}
	srintf("\t got mem: %x\n", table_mem);

	for(int i = 0; i < n_tables; i++){
		for(int j = 0; j < 1024; j++){
			table_mem[i * 1024 + j] = 0;
		}
	}

	for(int i = 0; i < n_tables; i++)
		directory->directory_v[tables_end + i] = &table_mem[1024 * i];

	directory->num_tables += n_tables;

	srintf(" succeded\n");
	return 0;
}

// void *get_page(unsigned int n_pages, unsigned int *error){
void *get_page_d(unsigned int n_pages, unsigned int map, unsigned int *error, int line, char* file){
	
	unsigned int found = 0;
	unsigned int j = find_block_in_bitmap(memory_bitmap, PAGE_BITMAP_LENGTH, n_pages, &found);
	
	if(found != 0){
		if(error != NULL)
			*error = 1;

		return NULL;
	}

	if(map == GET_PAGE_NOMAP){
		for(int i = 0; i < n_pages; i++){
			set_bit_in_bitmap(memory_bitmap, j + i);
		}

		srintf("[GET PAGE] Unmapped page got: n: %d, start: %x, at %s:%d\n", n_pages, j * 0x1000, file, line);

		free_memory -= n_pages;
		return (void *)(j * 0x1000);
	}

	found = 0;
	unsigned int pte = find_block_in_bitmap(k_virtual_bitmap, PAGE_BITMAP_LENGTH, n_pages, &found);

	if(found){
		if(error != NULL)
			*error = 2;
		printf("no v\n");
		return 0;
	}

	srintf("[GET PAGE] j = %d\n", j);
	int res = map_pages(&kernel_directory_struct, (pte / 1024) << 22 | ((pte % 1024) << 12), j * 0x1000, n_pages, PAGE_PRESENT_RW);
	if(res != 0){
		printf("[GET PAGE] map pages res == %d\n", res);
	}


	for(int i = 0; i < n_pages; i++){
		set_bit_in_bitmap(memory_bitmap, j + i);
	}

	// znajdowanie wolnej strony w katalogu
	// struct list_node *node = kernel_directory_struct.table_list->head;
	// int pde = pte / 1024;

	// // sprawdzanie czy blok stron będzie znajdował się na dwóch tabelach
	// // w takim przypadku trzeba zwiększyć ilość potrzebnych tabel i w razie czego je zaalokować
	// unsigned int tables_required = CEIL(n_pages, 1024);
	// int next_mul = pte + 1024 - (pte % 1024);
	// tables_required += next_mul < (pte + n_pages);

	// // jeżeli brakuje tabel, to trzeba dodać więcej
	// if(kernel_directory_struct.num_tables < pde + tables_required){
	// 	int res = alloc_tables(&kernel_directory_struct, tables_required);
	// 	if(res != 0){
	// 		printf("[GET PAGE] page not alloced\n");
	// 		if(error != NULL)
	// 			*error = 2;
	// 		return 0;
	// 	}
	// }



	// unsigned int v_addr = (pte / 1024) << 22 | (pte % 1024) << 12;
	// for(int i = 0; i < n_pages; i++, pte++){
	// 	unsigned int t_addr = (pte / 1024) << 22 | (pte % 1024) << 12;

	// 	int map_stat = map_phys_to_virt(&kernel_directory_struct, (j + i) * 0x1000, t_addr, PAGE_PRESENT_RW);
	// 	if(map_stat != 0){
	// 		if(error != NULL){
	// 			*error = 3 | map_stat << 8;
	// 		}

	// 		return 0;
	// 	}

	// 	set_bit_in_bitmap(memory_bitmap, j + i);
	// 	set_bit_in_bitmap(k_virtual_bitmap, pte);
	// }

	free_memory -= n_pages;

	flush_tlb();

	unsigned int v_addr = (pte / 1024) << 22 | ((pte % 1024) << 12);

	srintf("[GET PAGE] Mapped page got: n: %d, start: %x, at %s:%d\n", n_pages, v_addr, file, line);

	return (void *)v_addr;
}

// int map_pages(struct page_directory_struct *directory, void *v_addr, unsigned int phys_start, unsigned int n_pages, unsigned int flags){
int map_pages_d(struct page_directory_struct *directory, void *v_addr, unsigned int phys_start, unsigned int n_pages, unsigned int flags, int line, char *file){
	if(directory == NULL || n_pages == 0)
		return 1;
	srintf("[MAP PAGES D] mapping %x to %x len: %d, at %s:%d\n", phys_start, v_addr, n_pages, file, line);

	unsigned int uaddr = v_addr;

	// znajdowanie wolnej strony w katalogu
	// struct list_node *node = kernel_directory_struct.table_list->head;
	int pde = uaddr >> 22;
	int pte = pde * 1024 + ((uaddr >> 12) & 0x3FF);

	// sprawdzanie czy blok stron będzie znajdował się na dwóch tabelach
	// w takim przypadku trzeba zwiększyć ilość potrzebnych tabel i w razie czego je zaalokować
	unsigned int tables_required = CEIL(n_pages, 1024);
	int next_mul = pte + 1024 - (pte % 1024);
	tables_required += next_mul < (pte + n_pages);

	unsigned int tables_missing = 0;
	int missing_start = -1;
	for(int i = 0; i < tables_required; i++){
		if(directory->directory_v[pde + i] == NULL){
			tables_missing++;
			if(missing_start == -1){
				missing_start = pde + i;
			}
		}
	}
	srintf("[MAP PAGES] tables missing: %d, at: %d\n", tables_missing, tables_required);

	// jeżeli brakuje tabel, to trzeba dodać więcej
	// if(directory->num_tables < pde + tables_required){
	if(missing_start != -1){
		srintf("[MAP PAGES] allocing tables\n");
		int res = alloc_tables_at(directory, missing_start, tables_missing);
		// int res = alloc_tables(directory, tables_missing);
		if(res != 0){
			printf("[MAP PAGES] page not alloced: %d\n", res);
			return 2;
		}
		srintf("[MAP PAGES] tables alloced\n");
	}

	unsigned int bit_p = phys_start / 0x1000;
	// unsigned int bit_v = (pde * 1024) + pte;
	unsigned int bit_v = pte;
	srintf("[MAP PAGES] mapping %d pages, starting at phys: %x, v: %x\n", n_pages, phys_start, v_addr);

	for(int i = 0; i < n_pages; i++, bit_p++, bit_v++){
		// srintf("[MAP PAGES] mapping: %d\n", i);
		int map_res = map_phys_to_virt(directory, phys_start + i * 0x1000, uaddr + i * 0x1000, flags);
		if(map_res != 0){
			srintf("[MAP PAGES] phys to virt res: %d\n", map_res);
			unmap_pages(directory, v_addr, i);

			return 3;
		}

		// set_bit_in_bitmap(memory_bitmap, bit_p);
		set_bit_in_bitmap(directory->v_bitmap, bit_v);
		// srintf("[MAP PAGES] bit in bitmap set\n");
	}
	srintf("[MAP PAGES D] mapping at %s:%d succeded\n", file, line);

	return 0;
}

int unmap_pages(struct page_directory_struct *directory, void *v_addr, unsigned int n_pages){
	if(directory == NULL || n_pages == 0 || directory->directory_addr == NULL || directory->directory_v == NULL || directory->v_bitmap == NULL)
		return 1;

	// sprawdzanie czy szukane strony są zamapowane
	unsigned int uaddr = v_addr;
	unsigned int pde = uaddr >> 22;
	unsigned int j = (uaddr >> 22) * 1024 + ((uaddr >> 12) & 0x3FF);
	for(int i = 0; i < n_pages; i++, j++){
		unsigned int *table = directory->directory_v[j / 1024];
		if(table[j % 1024] == 0){
			return 2;
		}
	}

	// wszystko ok
	j = (uaddr >> 22) * 1024 + ((uaddr >> 12) & 0x3FF);
	for(int i = 0; i < n_pages; i++){
		if(unmap_virt_addr(directory, uaddr + i * 0x1000, 0) != 0){
			printf("[UNMAP PAGES] error unmapping\n");
			return 3;
		}
		clear_bit_in_bitmap(directory->v_bitmap, j + i);
	}

	return 0;
}

// int free_page(struct page_directory_struct *directory, void *v_addr, unsigned int n_pages)
int free_page_d(struct page_directory_struct *directory, void *v_addr, unsigned int n_pages, int line, char *file){
	if(directory == NULL || n_pages == 0 || directory->directory_addr == NULL || directory->directory_v == NULL || directory->v_bitmap == NULL)
		return 1;

	srintf("[FREE PAGE] freeing dir: %x, v_addr: %x, n_pages: %d, at %s:%d\n", directory, v_addr, n_pages, file, line);
	// sprawdzanie czy szukane strony są zamapowane
	unsigned int uaddr = v_addr;
	unsigned int pde = uaddr >> 22;
	unsigned int j = (uaddr >> 22) * 1024 + ((uaddr >> 12) & 0x3FF);

	// printf("[FREE PAGE] pde: %d, pte: %d, j: %d, n: %d\n", pde, (uaddr >> 12) & 0x3FF, j % 1024, n_pages);
	// printf("[FREE PAGE] uaddr: %x\n", uaddr);
	for(int i = 0; i < n_pages; i++, j++){
		unsigned int *table = directory->directory_v[j / 1024];
		if(table[j % 1024] == 0){
			return 2;
		}
	}

	j = (uaddr >> 22) * 1024 + ((uaddr >> 12) & 0x3FF);
	unsigned int block_start = get_phys_addr(directory, v_addr, NULL) / 0x1000;
	if(block_start == 0){
		printf("[FREE PAGE] block start == 0\n");
	}

	// printf("[FREE PAGE] block start: %d\n", block_start);

	for(int i = 0; i < n_pages; i++){
		clear_bit_in_bitmap(memory_bitmap, block_start + i);
	}

	int res = unmap_pages(directory, v_addr, n_pages);
	if(res != 0){
		return 3;
	}

	free_memory += n_pages;

	srintf("[FREE PAGE] n_pages: %d, at %s:%d\n", n_pages, file, line);

	return 0;
}

// int free_page(struct page_directory_struct *directory, void *v_addr, unsigned int n_pages){
// 	if(directory == NULL || npages == 0)
// 		return 1;

// 	unsigned int uvaddr = (unsigned int)v_addr;

// 	unsigned int pde = uvaddr >> 22;
// 	unsigned int pte = (uvaddr >> 12) & 0x3FF;

// 	unsigned int found;
// 	for(int i = 0; i < n_pages; i++, pte++){
// 		found = 0;
// 		unsigned int t_addr = (pde + (pte / 1024)) << 22 | (pte % 1024) << 12;
// 		unsigned int phys_addr = get_phys_addr(directory, (unsigned int)t_addr, &found);
// 		if(found != 0){
// 			return 2;
// 		}
// 	}

// 	pte = (uvaddr >> 12) & 0x3FF;
// 	for(int i = 0; i < n_pages; i++, pte++){
// 		unsigned int t_addr = (pde + (pte / 1024)) << 22 | (pte % 1024) << 12;
// 		unsigned int phys_addr = get_phys_addr(directory, (unsigned int)t_addr, NULL);

// 		int map_stat = unmap_virt_addr(directory, t_addr, PAGE_PRESENT_RW);
// 		if(map_stat != 0){
// 			return 3;
// 		}

// 		clear_bit_in_bitmap(memory_bitmap, phys_addr / 4096);
// 		// clear_bit_in_bitmap(k_virtual_bitmap, pte);
// 	}

// 	free_memory += n_pages;

// 	return 0;
// }

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
	// printf("Last kernel page: %d\n", last_kernel_page);
	total_memory += last_kernel_page;
	printf("free_memory: %d, total_memory: %d\n", free_memory, total_memory);

	// mapowanie kernela do katalogu stron
	kernel_directory[0] = (unsigned int)kernel_page1 | 3;
	for(int i = 1; i < 1024; i++){
		kernel_directory[i] = 0;
		kernel_directory_v[i] = 0;
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
	kernel_directory_struct.directory_phys_addr = kernel_directory;
	kernel_directory_struct.directory_addr = kernel_directory;
	kernel_directory_struct.directory_v = kernel_directory_v;
	kernel_directory_struct.num_tables = 1;
	kernel_directory_v[0] = kernel_page1;
	kernel_directory_struct.v_bitmap = k_virtual_bitmap;


	set_page_directory(kernel_directory);
	enable_paging();

	unsigned int page_got = 0;

	kernel_heap = get_page(KERNEL_HEAP_PAGES, GET_PAGE_MAP, &page_got);

	// printf("kernel heap: %x\n", kernel_heap);

	unsigned int *tabl = kernel_directory_struct.directory_v[(unsigned int)kernel_heap >> 22];

	// serial_write("mem bitmap, before free:\n");
	// for(int i = 0; i < 1024; i++){
	// 	serial_x(tabl[i]);
	// 	serial_write("\n");
	// }

	// printf("free res: %d\n", free_page(&kernel_directory_struct, kernel_heap, KERNEL_HEAP_PAGES));

	// serial_write("mem bitmap, after free:\n");
	// for(int i = 0; i < 1024; i++){
	// 	serial_x(tabl[i]);
	// 	serial_write("\n");
	// }

	// printf("kernel_heap: %x, %d\n", kernel_heap, page_got);
	if(!page_got){
		heap_init(kernel_heap, 0x1000 * KERNEL_HEAP_PAGES);
	}

	// printf("heap initialized\n");

	alloc_tables(&kernel_directory_struct, 2);
	// printf("[ - DEBG - ] last_kernel_page: %d, kend: %x\n", last_kernel_page, kernel_end);

	// alloc_tables(&kernel_directory_struct, 1020);
	// enlarge_heap(kernel_heap, 512 * 0x1000);
	// heap_all_info_c(kernel_heap);

	// kernel nie powinien mieć więcej niż 1GB, więc wydaje mi się, że tyle tabel mu wystarczy
	// alloc_tables(&kernel_directory_struct, 256 - 3);
	
	printf("Memory initiated\n");

	return 0;
}

// success == 0, jeżeli szukanie zakończyło się powodzeniem
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

// pokazuje zawartość tabel w katalogu dla danego bloku pamięci
void display_relevant_tables(struct page_directory_struct *directory,  void *addr, unsigned int length){
	if(addr != NULL && directory != NULL){
		unsigned int uaddr = (unsigned int)addr;
		unsigned int pde = uaddr >> 22;
		unsigned int pte = (uaddr >> 12) & 0x3FF;
		unsigned int end = uaddr + length;
		srintf("Displaying tables for %x - %x\n", uaddr, end);
		while(uaddr < end){
			if(pte >= 1024){
				pde++;
				pte = 0;
			}

			unsigned int table_val = 0;
			if(directory->directory_v[pde] != 0){
				table_val = ((unsigned int *)directory->directory_v[pde])[pte];
				srintf("[%d][%d] %x\n", pde, pte, table_val);
			}else{
				srintf("[%d] table == NULL\n", pde);
			}


			pte++;
			uaddr += 0x1000;
		}
	}
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