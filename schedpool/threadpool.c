#include "threadpool.h"

static void register_pool(Pool** p){
	pool_node* new = (pool_node*)malloc(sizeof(pool_node));
	new->next = NULL;
	new->pool = *p;
	pool_node** curr = &pool_registry;
	if(!pool_registry){
		*curr = new;
		return;
	}
	while((*curr)->next != NULL)*curr = (*curr)->next;
	*curr = new;
}

static void unregister_pool(Pool** p){
    pool_node* curr = pool_registry;
	if(curr == NULL)return;
	if(curr->pool == *p){
		pool_registry = curr->next;
		free(curr);
		return;
	}
	pool_node* prev = NULL;
    while (curr != NULL && curr->pool != *p){ 
        prev = curr; 
        curr = curr->next; 
    }
    if (curr == NULL) return; 
    prev->next = curr->next; 
    free(curr);	
}

// finds the threadpool and the id of the thread running this channel
void match_registered_pool(Pool** p, int* thread_id){
    // get thread_id;
    pthread_t currthread = pthread_self();
    // match this with current threadpool to get thread_id;    
    pool_node* curr = pool_registry;
	    while(curr != NULL){
			pthread_t* threadptr = curr->pool->threads_pointer;
			int pool_size = curr->pool->pool_size;
			// heap grows downward, therefore address of last thread created will be lower than
			// threadptr. minptr point to the last thread created by this pool
			pthread_t* minptr = threadptr + (pool_size- 1)*(sizeof(pthread_t));
			flog(DEBUG_REGISTRY, "\nthreadptr  %p\nmaxthreadptr %p\n", (void*)*threadptr, (void*)*minptr);
		// confirm that currthread is in the contigous memory created for the threads in this pool
		// size of heap alloc'd for these threads can vary,
		// the thread_id is found with this taken into account.
        if( currthread <= *threadptr && currthread >= *minptr){
			*thread_id = (*threadptr - currthread) /
				((*threadptr - *minptr) / (pool_size - 1));
			flog(DEBUG_REGISTRY, "found currthread %p, thread_id %p / %p = %d \n",
				(void*)currthread, (void*)(*threadptr - currthread),
				(void*)((*threadptr - *minptr) / (pool_size - 1) ), *thread_id );
            break;
        }
        curr = curr->next;
    }
    *p = curr->pool;
}



static void* pull_from_queue(void* arg){
	Args* args = (Args*)arg;
	int thread_id = args->thread_id;
	Pool* pool = args->pool;
	char *act = (pool->thread_active + thread_id);
	//pop node from queue
	xNode* queue_item;

	// register return env with scheduler
	jmp_buf* env = (jmp_buf*)malloc(sizeof(jmp_buf));
	if (setjmp(*env) == JMP_TO_POOL){
		flog(DEBUG_SCHED, "thread_id %d jumped back\n", thread_id);
		goto startpulling;
	}
	// store reference to this env in the env_store
	jmp_buf** iter = &(pool->chan_scheduler->env_store);
	*(iter+thread_id*sizeof(jmp_buf))= env;

	while (1){	
startpulling:
		pthread_mutex_lock(&(pool->queue_guard_mtx));
		while (!pool->queue.tail && !(*act)){
 			log(DEBUG_THREADPOOL, "thread %d is sleeping\n", thread_id);
				if (!pool->remaining_work && !pool->queue.tail){
					//if all work from other threads has been done
					pthread_cond_signal(&(pool->block_main));
				}
			pthread_cond_wait(&pool->hold_threads, &(pool->queue_guard_mtx));
		}
		
		queue_item = pop_node_queue(&(pool->queue));
		pthread_mutex_unlock(&(pool->queue_guard_mtx));

		if(queue_item){
			QueueObj* q_obj;
			q_obj = (QueueObj*)queue_item->data;
			function_ptr f = q_obj->func;
			void* args = q_obj->args;
			log(DEBUG_THREADPOOL, "thread %d is working\n", thread_id);		
			(*f)(args);
			log(DEBUG_THREADPOOL, "thread %d has finished working\n", thread_id);
			free(q_obj);
			pool->remaining_work--;
			*act = 0;
		}
		//break if signalled to terminate on empty queue and queue is empty			
		if (pool->exit_on_empty_queue && !pool->queue.tail && !pool->updating_queue)break;

	}
	pool->living_threads--;
	log(DEBUG_THREADPOOL, "\033[1;31mExiting thread %d \033[0m\n", thread_id  );
	pthread_exit(NULL);
}

void push_to_queue(Pool* pool, function_ptr f, void* args, char block){
	//this function is will be used to store a pointer to a function in a queue
	//pointers must point to function of type void, which takes in a void*
	
	pthread_mutex_lock(&(pool->queue_guard_mtx));		// lock mutex to access queue

	pool->remaining_work++;
	
	QueueObj* insert = (QueueObj*)malloc(sizeof(QueueObj));
	insert->func =f;
	insert->args = args;  //this function can be called with:(*insert->func)(insert->args);
	add_node((void*)insert ,&(pool->queue));		//push into pool queue 
	int i;
	for (i = 0; i < pool->pool_size; i++){
		if(*(pool->thread_active + i)){
			*(pool->thread_active + i) = 1;		//only wake one thread at a time
			break;
		}
	}
	pthread_mutex_unlock(&(pool->queue_guard_mtx));
	log(DEBUG_THREADPOOL,"\033[1;32mfinished pushing\033[0m\n");
	if(block){
		log(DEBUG_THREADPOOL, "\nblocking main\n");
		pthread_cond_broadcast(&(pool->hold_threads));
		while(pool->remaining_work ){
			pthread_cond_wait(&(pool->block_main), &(pool->mtx_spare));
		}
		
		pthread_mutex_unlock(&(pool->mtx_spare));
		log(DEBUG_THREADPOOL, "unblocking main\n\n");
		pool->updating_queue = 0;
		pool->living_threads = pool->pool_size;
		
		//now signal all threads which are not active to advance them past the cond_wait() block
		if(pool->exit_on_empty_queue){
			//wait for threads to exit if needed
			memset(pool->thread_active, 1, pool->pool_size*sizeof(char));	//mark all threads as active to escape while loop
			while(pool->living_threads)pthread_cond_broadcast(&(pool->hold_threads));
		}
	}
}

void prepare_push(Pool* pool, char exit_on_empty_queue){
	pool->exit_on_empty_queue = exit_on_empty_queue;
	pool->updating_queue = 1;
}

void init_pool(Pool* pool, char pool_size){	
	// this function initialises a threadpool pointer;
	pool->pool_size = pool_size;
	pool->remaining_work = 0;
	//initialise the linkedlist queue used to push arguemnts into the pool
	init_xLinkedList(&(pool->queue));
	
	// initialise scheduler used for channels
	sched_init(&(pool->chan_scheduler), pool_size);

	//initialse bool to mark whether or not the threads should terminate after the queue is empty
	pool->updating_queue = 0;
	 
	//initialise the mutex that the main thread will use to guard items being pulled from this queue
	pthread_mutex_init(&(pool->queue_guard_mtx), NULL);
	
	//initialise variables for blocking main
	pthread_mutex_init(&(pool->mtx_spare), NULL);
	pthread_cond_init(&(pool->block_main), NULL);
	pthread_cond_init(&(pool->hold_threads), NULL);
	//initialise threads and cond variable
	pool->threads_pointer = (pthread_t*)malloc(pool->pool_size * (sizeof(pthread_t)));
	// init all threads to be marked as not running
	pool->thread_active = (char*)calloc(pool->pool_size, pool->pool_size * sizeof(char));	
	
	pthread_t* thr_p = pool->threads_pointer;
	//create pthread with pointer to pull function	
	//initialise cond variables for all threads
	int i;
	for(i=0; i < pool->pool_size; i++){		
		Args* new_args = (Args*)malloc(sizeof(Args));		//struct for args
		new_args->thread_id = i;
		new_args->pool = pool;
		log(DEBUG_THREADPOOL, "created thread id = %d\n", i);

		pthread_create(thr_p, NULL, pull_from_queue, (void *)(new_args));	// create threads

		while((pool->hold_threads).__align == 2*i);		
		// this ensures that threads are created in order and each thread is created properly
		//(pool->hold_threads).__align is incremented by 2 for every thread created.
		
		//advance pointer
		thr_p += sizeof(pthread_t);
	}
	
	// add pool to the registry of all pools being used
	register_pool(&pool);
}

void cleanup(Pool* pool){
	//free() up memory that has been malloced
	free(pool->threads_pointer);
	free(pool->thread_active);
	unregister_pool(&pool);
}


/*
void join_pool(Pool* pool){
	//used for testing
	pthread_t* thr_p = pool->threads_pointer;
	int i;
	for(i=0; i < pool->pool_size; i++){		
		pthread_join((*thr_p), NULL);
		thr_p += sizeof(pthread_t);
	}
}*/