#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "chan.h"

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
	// sleep just to ensure that chan_funcA is forced to push into full channel
	sleep(3);
	chan* channel = (chan*)args;
	int* first;
	first = (int*)ch_pull(channel);
	printf("B pulled first=%d\n", *first);
	free(first);
	int* second;
	second = (int*)ch_pull(channel);
	printf("B pulled second=%d\n", *second);
	free(second);
}

int main(){
	srand(999);

	Pool* threadpool = (Pool*)malloc(sizeof(Pool));
	init_pool(threadpool, 3);   //pool_size = 3 

	chan* channel;
	channel = ch_make(0);   // unbuffered channel
	/*
	To create a buffered channel suitable here, change this to:
		channel = ch_make(1);  

	Tthe current unbuffered channel here will result in a segfault.
	(due to the stack unwinding issue outlined in README.md)
	*/

	prepare_push(threadpool, 1);
	push_to_queue(threadpool, chan_funcA, (void*)(channel), 0);
	push_to_queue(threadpool, chan_funcB, (void*)(channel), 1);
	ch_close(channel);

	cleanup(threadpool);	

	return 0;
}
