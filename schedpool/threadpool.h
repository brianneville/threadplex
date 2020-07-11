#ifndef C_THREADPOOL_H
#define C_THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "xor_LL.h"
#include "sched.h"

typedef struct Pool{
	int pool_size;                    // size of threadpool
	pthread_t* threads_pointer;       // used to iterate through threads	
	char* thread_active;              // bools marked as true if the thread is running
	xLinkedList queue;                // queue used to push items into the pool	
	pthread_mutex_t queue_guard_mtx;  // mtx to guard queue for consumers
	int living_threads;               //used to track if all threads have exited		
	pthread_cond_t block_main;        //used to block main thread while other threads execute
	sched* chan_scheduler;	          // scheduler for channel operations for this threadpool
	hashtable* origin_thread_ids;     // this tracks the original thread which is running the function

	//if true, then the threads will terminate after the queue has been filled and then emptied
	char exit_on_empty_queue;
	
	//set this to 1 before using the push_to_queue() function, and set it to 0 afterwards
	//this is used to tell the threads pulling from queue that other objects are going to be		
	//inserted into the queue.
	//(prevents issue where thread could finish work before next function has been added to queue,
	// while exit_on_empty_queue is set to 1. this would cause premature exit).		
	char updating_queue; 		
	
	//used to check if the queue is empty.
	// empty queue implies all functions are working or finished
	long remaining_work;		
	
	//spare, useless mutex for the condition variable (semaphores had reliability issues)	
	pthread_mutex_t mtx_spare;	
	
	//this condition variable makes threads sleep when finished working in an empty queue
	pthread_cond_t hold_threads;
}Pool;

typedef struct Args{
	int thread_id;
	Pool* pool;	
}Args;

typedef void (*function_ptr)(void* args);
typedef struct QueueObj{
	function_ptr func;
	void* args;
}QueueObj;

typedef struct pool_node{
	struct pool_node* next;
	Pool* pool;
}pool_node;

pool_node* pool_registry;
void match_registered_pool(Pool** p, int* thread_id);

//function calls for the user
// --
// initialise a new threadpool
void init_pool(Pool* pool,  char pool_size);

//call this to prepare the threadpool for pushing
void prepare_push(Pool* pool, char exit_on_empty_queue);

//push function to queue for pool. 
//set block == 1 to block in main thread
void push_to_queue(Pool* pool, function_ptr f, void* args, char block);	

//release all malloced memory back to the heap
void cleanup(Pool* pool);													

// functions for developer:
//void join_pool(Pool* pool);
#endif