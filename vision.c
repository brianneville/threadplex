#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "chan.h"

void test_func(void* args){
	printf("\t\tmodifying within passed function\n");
	int* arg = (int*)args;
	*arg = *arg + 1;
	//simulate work being done
	sleep(1+rand()%3);
	
}

void chan_funcA(void* args){
    chan* channel = (chan*)args;
    int* first = (int*)malloc(sizeof(int));
	*first = 123;
    ch_push(channel, (void*)(first));
    printf("A pushed first=%d\n", *first);
	int* second = (int*)malloc(sizeof(int));
	*second = 456;
    ch_push(channel, (void*)(second));
    printf("A pushed second=%d\n", *second);
}

void chan_funcB(void* args){
	sleep(1);
    chan* channel = (chan*)args;
    int* first;
    first = (int*)ch_pull(channel);
    printf("B pulled first=%d\n", *first);
	free(first);
    int* second;
    second = (int*)ch_pull(channel);
    printf("pulled second=%d\n", *second);
	free(second);
}

int main(){
	srand(999);
	
	Pool* threadpool = (Pool*)malloc(sizeof(Pool));
	init_pool(threadpool, 3);	//pool_size = 3 

    chan* channel;
    channel = ch_make(0);

    prepare_push(threadpool, 0);
    push_to_queue(threadpool, chan_funcA, (void*)(channel), 0);
    push_to_queue(threadpool, chan_funcB, (void*)(channel), 1);
    ch_close(channel);


    
	/*
	int args = 0;
	printf("input to queue is %p with contents %d\n",&args, args);
	
	prepare_push(threadpool, 0);	//exit_on_empty_queue = 0 to reuse threadpool
	push_to_queue(threadpool, test_func, (void*)(&args), 0);
	push_to_queue(threadpool, test_func, (void*)(&args), 0);
	push_to_queue(threadpool, test_func, (void*)(&args), 0);
	push_to_queue(threadpool, test_func, (void*)(&args), 1);	//block = 1 on final push call
	
	printf("args is now %d\n", args);
	
	prepare_push(threadpool, 0);	//exit_on_empty_queue = 0
	push_to_queue(threadpool, test_func, (void*)(&args), 0);
	push_to_queue(threadpool, test_func, (void*)(&args), 0);
	push_to_queue(threadpool, test_func, (void*)(&args), 1);
	
	printf("args is now %d\n", args);
	
	prepare_push(threadpool, 1);	//exit_on_empty_queue = 1
	push_to_queue(threadpool, test_func, (void*)(&args), 1);
	
	printf("args is now %d\n", args);
	*/
	cleanup(threadpool);	
	
	
	return 0;
}
