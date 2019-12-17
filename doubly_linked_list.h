#ifndef _DOUBLY_LINKED_LIST_H_
#define _DOUBLY_LINKED_LIST_H_

#include <stddef.h>
#include "stdio.h"
// extern thread_t;
#define LIST_TYPE void *

struct lnode{
	struct lnode *next;
	struct lnode *prev;
	
	struct lnode *nextl;
	struct lnode *prevl;
	
	LIST_TYPE value;
	
	unsigned int used;
};

struct list_info{
	struct lnode *head;
	struct lnode *tail;
	
	unsigned int nodes;
	unsigned int free_nodes;

	int *(*get_num)(void *);
};

#define LIST_first_block(list) (struct lnode *)((char *)list + sizeof(struct list_info))
#define GET_VALUE(x) (*(list->get_num(x)))

struct lnode *find_free_block(struct list_info *list);
int push_back(struct list_info *list, LIST_TYPE value);
LIST_TYPE pop_front(struct list_info *list);
struct list_info *init_list(void *memory, unsigned int size);

// wstawia element do koleji w taki sposób, że suma wszystkich poprzednich elementóœ i jego jest równa num
int queue_num(struct list_info *list, LIST_TYPE num);
// dekrementuje wartość pierwszego elementu lub tylu elementów które mają taką samą wartość jak pierszy
int queue_dec(struct list_info *list);

void display_list_header(struct list_info *list);
void display_list(struct list_info *list);

#endif