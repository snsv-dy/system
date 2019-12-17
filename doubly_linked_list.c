#include "doubly_linked_list.h"

struct lnode *find_free_block(struct list_info *list){
	struct lnode *t = LIST_first_block(list);
	struct lnode *last = t;
	while(t != NULL && t->used){
		last = t;
		t = t->next;
	}
	
	if(t == NULL){
		//enlarge_list
		return NULL;
	}
	
	list->free_nodes--;
	
	return t;
}


int push_back(struct list_info *list, LIST_TYPE value){
	if(list == NULL){
		return 1;
	}
	
	struct lnode *t = find_free_block(list);
	if(t == NULL){
		return 2;
	}
	
	t->value = value;
	t->used = 1;
	
	if(list->head == NULL){
		list->head = list->tail = t;
		t->nextl = NULL;
		t->prevl = NULL;
		
		return 0;
	}
	
	list->tail->nextl = t;
	t->prevl = list->tail;
	t->nextl = NULL;
	list->tail = t;
	
	return 0;
}

LIST_TYPE pop_front(struct list_info *list){
	if(list == NULL || list->head == NULL){
		return 0;
	}
	
	struct lnode *t = list->head;
	t->used = 0;
	
	list->free_nodes++;
	
	if(list->head == list->tail){
		list->head = list->tail = NULL;
		return t->value;
	}
	
	list->head = t->nextl;
	list->head->prevl = NULL;
	
	return t->value;
}

struct list_info *init_list(void *memory, unsigned int size){
	if(memory == NULL || size < sizeof(struct list_info)){
		return NULL;
	}
	
	struct list_info *list = (struct list_info *)memory;
	
	unsigned int nodes = (size - sizeof(struct list_info)) / sizeof(struct lnode);
	struct lnode *first = (char *)memory + sizeof(struct list_info);
	for(int i = 0; i < nodes; i++){
		if(i > 0)
			first[i].prev = &first[i - 1];
		else
			first[i].prev = NULL;
		if(i < nodes - 1)
			first[i].next = &first[i + 1];
		else
			first[i].next = NULL;
			
		first[i].nextl = NULL;
		first[i].prevl = NULL;
			
		first[i].used = 0;
		first[i].value = (LIST_TYPE)0;
	}
	
	list->head = list->tail = NULL;
	list->nodes = nodes;
	list->free_nodes = nodes;
	
	return list;
} 

extern struct thread_t;
#define VALUE_TO_THREAD(x) ((thread_t *)x)
// #define GET_VALUE(x) (*(list->get_num(x)))
// wstawia element do koleji w taki sposób, że suma wszystkich poprzednich elementóœ i jego jest równa num
int queue_num(struct list_info *list, LIST_TYPE num){
	if(list == NULL)
		return 1;
		
	struct lnode *node = (struct lnode *)list->head;
	
	int tnum = GET_VALUE(num);
	int accumulated = 0;
	while(node != NULL && accumulated < tnum){
		if(accumulated + GET_VALUE(node->value) > tnum){
			break;
		}
		accumulated += GET_VALUE(node->value);
		node = node->nextl;
	}
	
	tnum = tnum - accumulated;

	if(node == NULL){
		GET_VALUE(num) = tnum;
		return push_back(list, num);
	}
		
	
	struct lnode *new_node = find_free_block(list);
	if(new_node == NULL)
		return 2;
	
	new_node->prevl = node->prevl;
	new_node->nextl = node;
	if(node == list->head){
		list->head = new_node;
	}
	
	if(new_node->nextl != NULL)
		new_node->nextl->prevl = new_node;
		
	if(new_node->prevl != NULL)
		new_node->prevl->nextl = new_node;
	
	new_node->value = num;
	GET_VALUE(new_node->value) = tnum;
	new_node->used = 1;
	
	if(tnum > 0){
		int tosub = GET_VALUE(node->value);
		while(node != NULL && GET_VALUE(node->value) == tosub){
			GET_VALUE(node->value) -= tnum;
			node = node->nextl;
		}
	}
	
	return 0;
}

// dekrementuje wartość pierwszego elementu lub tylu elementów które mają taką samą wartość jak pierszy
int queue_dec(struct list_info *list){
	struct lnode *node= list->head;
	if(node == NULL)
		return 1;
		
	int first_value = GET_VALUE(node->value);

	struct lnode *t = node;
	GET_VALUE(t->value)--;
	return 0;
}

void display_list_header(struct list_info *list){
	if(list != NULL){
		printf("-----\nhead: %x\ntail: %x\nnodes: %d\nfree nodes: %d\n-----\n", list->head, list->tail, list->nodes, list->free_nodes);
	}
}

void display_list(struct list_info *list){
	if(list != NULL){
		struct lnode *t = list->head;
		while(t != NULL){
			printf("next: %x\nprev: %x\nvalue: %x\nvalue value: %d\n-----\n", t->nextl, t->prevl, (unsigned int)t->value, GET_VALUE(t->value));
			t = t->nextl;
		}
	}
}