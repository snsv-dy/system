#ifndef _HEAP_H_
#define _HEAP_H_

#include "stdio.h"
#include <stddef.h>

// bloki których adres jest podzielny przez 4096 
// są początkami strony i stąd będzie wiadomo które strony uwolnić
// ewentualnie niszcząc stertę

#define CEIL(a, b) (((a) + (b) - 1) / (b))

typedef struct heap_block_s{
	struct heap_block_s *prev;
	struct heap_block_s *next;
	
	unsigned int length;
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
void memcpy_t(const void *src, void *dest, unsigned int len);
void *heap_realloc(heap_block *heap, void *addr, unsigned int new_size);
int check_heap_continuity(heap_block *heap);
unsigned int heap_available_memory(heap_block *heap);

#endif