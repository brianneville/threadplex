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
        // meaning that this function was woken up here
        pthread_mutex_lock(mtx);
        return;
    }
    pthread_mutex_lock(&(chan_scheduler->sched_lock));
    // register that this env is channel is trying to push at this env.
    // register that this env is a valid longjmp point only for this thread_id.
    // TODO: will this currenv pointer always be at different memory addresses? I think so.
    hashtable_insert(chan_scheduler->hash_push, key, (void*)currenv);
    hashtable_insert(chan_scheduler->env_to_thread, currenv, (void*)thread_id);

    // see if there are any attempts to pull from this channel in the hash_pull
    node* pull_attempt = hashtable_find(chan_scheduler->hash_pull, key);
    if (pull_attempt){
        // check if this pull attempt is in the same thread
        node* env_node = hashtable_find(chan_scheduler->env_to_thread, (void*)pull_attempt);
        if ((int)env_node->val == thread_id){
            log(DEBUG_SCHED, "pull attempt is in the same thread, jumping to it");
            pthread_mutex_unlock(&(chan_scheduler->sched_lock));
            longjmp(((jmp_buf*)(pull_attempt->val))[0], JMP_TO_CHPULL);
        }
    
        // else this pull attempt exists, but its being handled by a different thread.
        // TODO: 
        log(DEBUG_SCHED, "pull attempt is in another thread -  \
                marking this other env to be resumed by the other thread");
        // ....

       }  
    log(DEBUG_SCHED, "jumping to pull a new function from the queue");
    // Jump away to load a new function in this thread - perhaps this new function will 
    // pull an item from the channel
    // In the case that there are no more functions waiting to be run in the threadpool
    // deadlock will occur
    // TODO : what if a channel is doing operations from inside main thread?
    pthread_mutex_unlock(&(chan_scheduler->sched_lock));
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

    // lock for scheduling operations
    pthread_mutex_init(&(*s)->sched_lock, NULL);

    // create hashtable for storing the env of goroutines that call ch_push
    // create a separate hashtable for storing the env of goroutines that call ch_pull
    (*s)->hash_pull = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_modkey);
    (*s)->hash_push = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_modkey);

    // create a hashtable to map the env to the thread_id that it is running in.
    // used to ensure that setjmp does not jump outside its own thread.
    (*s)->env_to_thread = hashtable_create(HASHTABLE_SIZE, USE_CHAINING, hashfunc_sumpointer);
    // store for envs at start of threadpool pulling
    (*s)->env_store = (jmp_buf*)malloc(pool_size*sizeof(jmp_buf));

    // hashtable storing chain of functions that need to be pulled
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