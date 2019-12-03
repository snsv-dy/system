#ifndef _MEMORY2_H_
#define _MEMORY2_H_

#include <stddef.h>
#include "stdio.h"
#include "heap.h"

extern void set_page_directory(unsigned int *dir_addr);
extern void enable_paging();
extern void flush_tlb();

struct mb_mem_map{
	unsigned int size;
	unsigned int addr_low, addr_high;
	unsigned int length_low, length_high;
	unsigned int type;
} __attribute__((packed));
typedef struct mb_mem_map multiboot_memory_map;

// Dzielenie całkowite z zaokrągleniem do góry 
#define CEIL(a, b) (((a) + (b) - 1) / (b))

void set_bit_in_bitmap(unsigned int *bitmap, unsigned int pos);
void clear_bit_in_bitmap(unsigned int bitmap[], unsigned int pos);
unsigned int check_bit(unsigned int bitmap[], unsigned int pos);
unsigned int find_block_in_bitmap(unsigned int *bitmap, unsigned int bit_length, unsigned int size, unsigned int *success);

// lista tablic
struct list_node{
	struct list_node *prev;
	struct list_node *next;

	unsigned int *table_addr;
	unsigned int free_pages;	// nie obsługiwane
};

struct page_table_list{
	struct list_node *head;
	struct list_node *tail;

	unsigned int size;
};

// struktura opisująca katalog stron
struct page_directory_struct{
	unsigned int *directory_addr;
	struct page_table_list *table_list;
	unsigned int *bitmap;

	unsigned int reserve_next;
};

#define PAGES_NUM (0x100000000 / 0x1000)
// #define PAGE_BITMAP_LENGTH (PAGES_NUM / sizeof(unsigned int))
#define PAGE_BITMAP_LENGTH (PAGES_NUM / (sizeof(unsigned int) * 8))
// #define PAGE_BITMAP_LENGTH (0x8000)

#define PAGE_PRESENT_RW 3

unsigned int get_phys_addr(struct page_directory_struct *directory, unsigned int v_addr, unsigned int *success);
int map_phys_to_virt(struct page_directory_struct *directory, unsigned int phys_addr, unsigned int v_addr);
// void *get_page(unsigned int n_pages, unsigned int *error);
void *get_page_d(unsigned int n_pages, unsigned int *error, int line, char* file);
int free_page(void *v_addr, unsigned int n_pages);
int initialize_memory(unsigned int kernel_start, unsigned int kernel_end, multiboot_memory_map *mmap_addr, unsigned int mmap_length);
int enlarge_heap(heap_block *last_block, unsigned int size);
int alloc_tables(struct page_directory_struct *directory, unsigned int n_tables);

#endif