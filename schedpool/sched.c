#include "sched.h"

// jump execution to the point where the threadpool is about to pull another function 
static void jump_threadpull(sched* chan_scheduler, int thread_id){
    jmp_buf** ptr = &(chan_scheduler->env_store);
    longjmp(*(ptr+thread_id*sizeof(jmp_buf))[0], JMP_TO_POOL);
}

void printchan_sched(sched* chan_scheduler){
    printf("PULL\n");
    hashtable_print(chan_scheduler->hash_pull);
    printf("PUSH\n");
    hashtable_print(chan_scheduler->hash_push);
    printf("ENV_TO_ID\n");
    hashtable_print(chan_scheduler->env_to_id);
    printf("PENDING_ENVS\n");
    hashtable_print(chan_scheduler->pending_envs);
}

static void sched_executionredirect(sched* chan_scheduler, uint key, int thread_id,
    pthread_mutex_t* mtx, char JMP_ORIGIN, char JMP_DESTINATION, hashtable* chankey_to_currenv, 
    hashtable* opposite_chankey_to_currenv){
    jmp_buf* currenv = (jmp_buf*)malloc(sizeof(jmp_buf));
    pthread_mutex_unlock(mtx);
    if (setjmp(*currenv) & (JMP_ORIGIN | JMP_RESUME_PENDING)){
        log(DEBUG_SCHED, "jumped to ch%s", JMP_ORIGIN == JMP_TO_CHPULL ? "pull" : "push");
        pthread_mutex_lock(mtx);
        return;
    }
    pthread_mutex_lock(&(chan_scheduler->sched_lock));
    hashtable_insert(chankey_to_currenv, key, (void*)currenv);
    hashtable_insert(chan_scheduler->env_to_id, (void*)currenv, (void*)thread_id);

    // E.g. if action is PULL, then opposite_attempt represents an attempt to PUSH into a channel.
    node* opposite_attempt = hashtable_remove(opposite_chankey_to_currenv, key);
    log(DEBUG_SCHED, "thread_id = %d, opposite_attempt val = \
         %p \n\t(nil if opposite_attempt is nil)\n", thread_id, 
         opposite_attempt ? opposite_attempt->val : NULL);
    if (opposite_attempt){
        node* envid_node = hashtable_remove(chan_scheduler->env_to_id, 
            (unsigned long)opposite_attempt->val);
        log(DEBUG_SCHED, "envid node = %p\n", envid_node);
        if (envid_node->val == (void*)thread_id){
            log(DEBUG_SCHED, "opposite attempt is in the same thread, jumping to it\n");
            pthread_mutex_unlock(&(chan_scheduler->sched_lock));
            longjmp(((jmp_buf*)(opposite_attempt->val))[0], JMP_DESTINATION);
        }        
        log(DEBUG_SCHED, "opposite attempt is in another thread");
        hashtable_insert(chan_scheduler->pending_envs,
             (unsigned long)envid_node->val, (void*)opposite_attempt->val);
    }
    pthread_mutex_unlock(&(chan_scheduler->sched_lock));
    log(DEBUG_SCHED, "jumping to pull a new function from the queue\n");
    jump_threadpull(chan_scheduler, thread_id);
}

// pushing to a full channel.
// store env on hash_push, unlock mtx and jump away
void sched_pushing_full(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx){
    log(DEBUG_SCHED, "pushing full, sched is %p\n", (void*)chan_scheduler);
    sched_executionredirect(chan_scheduler, key, thread_id, mtx, JMP_TO_CHPUSH, JMP_TO_CHPULL,
    chan_scheduler->hash_push, chan_scheduler->hash_pull);
}

// pulling from an empty channel
void sched_pulling_empty(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx){
    log(DEBUG_SCHED, "pulling empty, sched is %p\n", (void*)chan_scheduler);
    sched_executionredirect(chan_scheduler, key, thread_id, mtx, JMP_TO_CHPULL, JMP_TO_CHPUSH,
    chan_scheduler->hash_pull, chan_scheduler->hash_push);
}

void sched_init(sched** s, int pool_size){
    *s = (sched*)malloc(sizeof(sched));

    // lock for scheduling operations
    pthread_mutex_init(&(*s)->sched_lock, NULL);

    // create hashtable for storing the env of goroutines that call ch_push
    // create a separate hashtable for storing the env of goroutines that call ch_pull
    (*s)->hash_pull = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_modkey);
    (*s)->hash_push = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_modkey);

    // create a hashtable to map the env to the thread_id that it is running in.
    // used to ensure that setjmp does not jump outside its own thread.
    (*s)->env_to_id = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_sumpointer);

    // hashtable for envs pending resumption for each thread.
    (*s)->pending_envs = hashtable_create(pool_size, USE_CHAINING, hashfunc_modkey);

    // store for envs at start of threadpool pulling
    (*s)->env_store = (jmp_buf*)malloc(pool_size*sizeof(jmp_buf));
}

void sched_destroy(sched* s){
    hashtable_cleanup(s->hash_pull);
    hashtable_cleanup(s->hash_push);
    hashtable_cleanup(s->env_to_id);
    hashtable_cleanup(s->pending_envs); 
    free(s->env_store);
}
