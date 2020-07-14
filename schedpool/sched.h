#ifndef SCHED_H
#define SCHED_H

#ifndef HASHTABLE_SIZE
#define HASHTABLE_SIZE 10
#endif
// tracing the origin of the longjmp
#define JMP_TO_POOL         0x1
#define JMP_TO_CHPULL       0x2
#define JMP_TO_CHPUSH       0x4
#define JMP_RESUME_PENDING  0x8

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
    hashtable* env_to_id;

    // Map thread_id to a list of envs that are waiting to be resumed at the pull_from_queue func
    hashtable* pending_envs;

    pthread_mutex_t sched_lock;
}sched;

void sched_init(sched** s, int pool_size);
void sched_pushing_full(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx);
void sched_pulling_empty(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx);
void printchan_sched(sched* chan_scheduler);
#endif
