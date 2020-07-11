#include "chan.h"

//static hashtable* sched_thread_cache;

// value of zero will make channel unbuffered
chan* ch_make(int buffer){
    //intialise buffer
    chan* ch = (chan*)malloc(sizeof(chan));
    ch->cap = buffer;
    ch->buffer = (void**)calloc(1+ch->cap, sizeof(void*));
    ch->id = maxID;
    ch->buf_head = -1;
  	pthread_mutex_init(&ch->lock, NULL);
    maxID++;
    
    pthread_mutex_lock(&ch->lock);
    // if (sched_thread_cache == NULL){
    //     sched_thread_cache = hashtable_create(IDCACHE_SIZE, NO_CHAINING);
    // }
    pthread_mutex_unlock(&ch->lock);

    return ch;
}

void ch_push(chan* ch, void* item){
    pthread_mutex_lock(&ch->lock);

    //hashtable_insert(sched_thread_cache, pthread_self(), ())
    if (ch->buf_head >= ch->cap){
        int thread_id;
        Pool* pool;
        match_registered_pool(&pool, &thread_id);
        /* pushing into a channel which is full */
        // TODO some check to see if this pthread_self->(scheduler+thread_id) pair is cached
        sched_pushing_full(pool->chan_scheduler, ch->id, thread_id, &ch->lock);
        flog(DEBUG_SCHED, "escaped sched_pushing_full");

    }
    // add item to the buffer
    // wake any goroutines which are waiting to pull from this channel
    ch->buf_head++;
    (ch->buffer)[ch->buf_head] = item;
    pthread_mutex_unlock(&ch->lock);
    flog(DEBUG_SCHED, "returning from push\n");
}

void* ch_pull(chan* ch){
    pthread_mutex_lock(&ch->lock);
    if (ch->buf_head == -1){
        int thread_id;
        Pool* pool;
        match_registered_pool(&pool, &thread_id);
       /* pulling from a channel which is empty */
        sched_pulling_empty(pool->chan_scheduler, ch->id, thread_id, &ch->lock);
    }
    // pull from the buffer
    // wake any functions which are waiting to push into this channel
    void* result = (ch->buffer)[ch->buf_head];
    ch->buf_head--;
    pthread_mutex_unlock(&ch->lock);
    return result;
}

// close a channel
void ch_close(chan* ch){};


