#ifndef _HEAP_H_
#define _HEAP_H_

#include "util.h"
#include "stdio.h"
#include <stddef.h>

// bloki których adres jest podzielny przez 4096 
// są początkami strony i stąd będzie wiadomo które strony uwolnić
// ewentualnie niszcząc stertę

#define CEIL(a, b) (((a) + (b) - 1) / (b))

typedef struct heap_block_s{
	struct heap_block_s *prev;
	struct heap_block_s *next;
	
	unsigned int dead;
	unsigned int length;
	unsigned int daed;
	unsigned int used : 1;
} heap_block;

int enlarge_heap(heap_block *last_block, unsigned int size);
void heap_info(void *heap);
heap_block *heap_init(void *heap, unsigned int size);
heap_block *split_heap(heap_block *heap, unsigned int size);
heap_block *merge_heap(heap_block *heap);
void display_heap(heap_block *heap);
void *heap_malloc(heap_block *heap, unsigned int size);
void *heap_malloc_page_aligned(heap_block *heap, unsigned int size);
void *heap_calloc(heap_block *heap, unsigned int num, unsigned int size);
void heap_free(heap_block *heap, void *addr);
void *heap_realloc(heap_block *heap, void *addr, unsigned int new_size);
int check_heap_continuity(heap_block *heap);
unsigned int heap_available_memory(heap_block *heap);
void heap_info_c(heap_block *node);
void heap_all_info_s(heap_block *node);
heap_block *get_kernel_heap();

#define kmalloc(size) heap_malloc(get_kernel_heap(), size)
#define kmalloc_page_aligned(size) heap_malloc_page_aligned(get_kernel_heap(), size)
#define kcalloc(num, sizet) heap_calloc(get_kernel_heap(), num, sizet)
#define krealloc(addr, newsize) heap_realloc(get_kernel_heap(), addr, newsize)
#define kfree(addr) heap_free(get_kernel_heap(), addr)

#endif