#include "xor_LL.h"

void init_xLinkedList(xLinkedList* list){
	list->tail = NULL;
	list->head = NULL;
}

void add_node(void* data, xLinkedList* list){

	xNode* new_xNode = (xNode* )malloc(sizeof(xNode));
	new_xNode->data = data;
	
	unsigned long prev_addr = (unsigned long) list->tail;
	unsigned long next_addr = (unsigned long)(new_xNode);
	new_xNode->address_ptr = prev_addr;    
	
	if(!list->tail){
		log(DEBUG_XOR_LL, "adding first xNode\n");
		new_xNode->address_ptr = (unsigned long)NULL;
		list->tail = new_xNode;
		list->head = new_xNode;
	}
	else if(!list->tail->address_ptr){
		log(DEBUG_XOR_LL, "adding second xNode\n");     
		list->tail->address_ptr = next_addr; 
	}
	else{
		log(DEBUG_XOR_LL, "adding a new xNode\n");
		list->tail->address_ptr = next_addr ^ list->tail->address_ptr;
	}
	
	list->tail = new_xNode;	
}

void traverse_list(xLinkedList* list){
	//start at tail and traverse in reverse
	xNode* curr = list->tail;
	xNode* next;
	xNode* prev = NULL;
	printf("Traversing list in reverse\n");
	if(!curr)printf("this list is empty\n");
	while (curr){
		printf ("data is: %p \n", curr->data);
		next = (xNode*)((unsigned long)prev ^ (unsigned long)curr->address_ptr);
		printf ("prev is %p, curr is %p, next is %p, address_ptr is %p\n",prev, curr, next, (void*)curr->address_ptr);
		prev = curr;
		curr = next;
	}
}

void delete_head_func(xLinkedList* list){
	xNode* curr = list->head;
	//xNode* next = NULL;
	xNode* prev = (xNode*)((unsigned long)(curr->address_ptr));
	log(DEBUG_XOR_LL, "list contains more than one xNode. deleting head\n");
	//now go back to the previous node. set the prev node's address pointer to point only back to the node before it;
	//therefore the head node can now be popped from the list
	xNode* before_prev = (xNode* )((unsigned long)curr ^ (unsigned long)prev->address_ptr); 
	prev->address_ptr = (unsigned long)before_prev;
	list->head = NULL;
	free(list->head);
	list->head = prev;
}

void delete_tail_func(xLinkedList* list){
	xNode* curr = list->tail;
	xNode* next;
	xNode* prev = NULL;
	log(DEBUG_XOR_LL, "list contains more than one xNode. deleting tail\n");
	next = (xNode*)((unsigned long)prev ^ (unsigned long)curr->address_ptr);
	//go to next xNode
	//set address_ptr at next xNode to point to the address of the next xNode it needs to go to
	//prev will be set to NULL
	prev = curr;
	curr = next;
	next->address_ptr = (unsigned long)((unsigned long)prev ^ (unsigned long)curr->address_ptr);
	list->tail = NULL;
	free(list->tail);
	list->tail = next;
}


void delete_end(xLinkedList* list, int delete_head){		
//set delete_head to 1 if deleting head, else deleting tail
	//deletes the xNode for which next is NULL;
	// note this is done from the perspecive of traversing the list in reverse as before
	if (!list->tail){
		//list empty, head and tail are NULL
		log(DEBUG_XOR_LL, "list empty. unable to delete\n");
	}	
	else if(!list->tail->address_ptr){	
		log(DEBUG_XOR_LL, "list contains only one xNode. This is being deleted\n");     
		list->tail = NULL;
		free(list->tail);
		list->head = NULL;
		free(list->head);
	}else {
		if(delete_head){
			delete_head_func(list);
		}else{
			delete_tail_func(list);
		}

	}
}

xNode* pop_node_stack(xLinkedList* list){
	//pops the most recently added node
	//returns the tail node of the list
	// note this is done from the perspecive of traversing the list in reverse as before
	xNode* temp = list->tail;
	delete_end(list, 0);
	return temp;		
}

xNode* pop_node_queue(xLinkedList* list){
	xNode* temp = list->head;
	delete_end(list, 1);
	return temp;	
}

void reverse(xLinkedList* list){
	xNode* temp; 
	temp = list->head;
	list->head = list->tail;
	list->tail = temp;
}
