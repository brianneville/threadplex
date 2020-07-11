#include "sched.h"

// jump execution to the point where the threadpool is about to pull another function 
static void jump_threadpull(sched* chan_scheduler, int thread_id){
    jmp_buf** ptr = &(chan_scheduler->env_store);
    longjmp(*(ptr+thread_id*sizeof(jmp_buf))[0], JMP_TO_POOL);
}

// pushing to a full channel.
// store env on hash_push, unlock mtx and jump away
void sched_pushing_full(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx){
    flog(DEBUG_SCHED, "pushing full, sched is %p\n", (void*)chan_scheduler);
    jmp_buf* currenv = (jmp_buf*)malloc(sizeof(jmp_buf));
    pthread_mutex_unlock(mtx);
    if (setjmp(*currenv) == JMP_TO_CHPUSH){
        flog(DEBUG_SCHED, "jumped to chpush\n");
        // non-zero in the event that this line is reached through a longjmp
        // meaning that this goroutine was woken up here
        //pthread_mutex_lock(mtx);
        return;
    }
    hashtable_insert(chan_scheduler->hash_push, key, (void*)currenv);

    // see if there are any attempts to pull from this channel in the hash_pull
    node* pull_attempt = hashtable_find(chan_scheduler->hash_pull, key);
    if (pull_attempt){
        longjmp(((jmp_buf*)(pull_attempt->val))[0], JMP_TO_CHPULL);
    }
    // Jump away to load a new function in this thread - perhaps this new function will 
    // pull an item from the channel
    // In the case that there are no more functions waiting to be run in the threadpool
    // deadlock will occur
    // TODO : what if a channel is doing operations from inside main thread?
    jump_threadpull(chan_scheduler, thread_id);
}

// pulling from an empty channel
void sched_pulling_empty(sched* chan_scheduler, uint key, int thread_id, pthread_mutex_t* mtx){
    flog(DEBUG_SCHED, "pulling empty, sched is %p\n", (void*)chan_scheduler);
    jmp_buf* currenv = (jmp_buf*)malloc(sizeof(jmp_buf));
    pthread_mutex_unlock(mtx);
    if (setjmp(*currenv) == JMP_TO_CHPULL){
        flog(DEBUG_SCHED, "jumped to chpull");
        pthread_mutex_lock(mtx);
        return;
    }
    hashtable_insert(chan_scheduler->hash_pull, key, (void*)currenv);
    node* push_attempt = hashtable_find(chan_scheduler->hash_push, key);
    if (push_attempt){
        longjmp(((jmp_buf*)(push_attempt->val))[0], JMP_TO_CHPUSH);
    }

    jump_threadpull(chan_scheduler, thread_id);
}

void sched_init(sched** s, int pool_size){
    *s = (sched*)malloc(sizeof(sched));
    // create hashtable for storing the env of goroutines that call ch_push
    // create a separate hashtable for storing the env of goroutines that call ch_pull
    (*s)->hash_pull = hashtable_create(HASHTABLE_SIZE, USE_CHAINING);
    (*s)->hash_push = hashtable_create(HASHTABLE_SIZE, USE_CHAINING);

    // store for envs at start of threadpool pulling
    (*s)->env_store = (jmp_buf*)malloc(pool_size*sizeof(jmp_buf));
}

void sched_destroy(sched* s){}


// void suspend(sched* s){
//     s->suspendedCount++;
//     realloc(s->suspendedContexts, s->suspendedCount * sizeof(ucontext_t));
//     goto 

// }

// void unsuspend(sched* s){
//     s->suspendedCount--;
//     realloc(s->suspendedContexts, s->suspendedCount * sizeof(ucontext_t));
//     // ucontext_t* ctx = 
//     // getcontext(ctx);

// }