#ifndef SCHED_H
#define SCHED_H

#ifndef HASHTABLE_SIZE
#define HASHTABLE_SIZE 10
#endif

#define JMP_TO_POOL 1
#define JMP_TO_CHPULL 2
#define JMP_TO_CHPUSH 3


#include <pthread.h>
#include <stdlib.h>
#include <setjmp.h>
#include "hash.h"
#include "logging.h"

// sched is responsible for interfacing between the 'ch_XXXX' functions,
// and the underlying threadpool implementation. 
typedef struct sched{
    hashtable* hash_pull;
    hashtable* hash_push;
    jmp_buf* env_store;     // store return points for starting to pull from queue
}sched;

void sched_init(sched** s, int pool_size);
void sched_pushing_full(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx);
void sched_pulling_empty(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx);

#endif
