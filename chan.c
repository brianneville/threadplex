#include "chan.h"

// value of zero will make channel unbuffered
// value of one will make the channel have a buffer for 1 item
//  (i.e. it can have 2 items in it - one in the channel, and one in the buffer)
chan* ch_make(int buffer){
    chan* ch = (chan*)malloc(sizeof(chan));
    ch->cap = buffer;
    ch->buffer = (void**)calloc(1+ch->cap, sizeof(void*));
    ch->id = maxID;
    ch->buf_head = -1;
    ch->pull_head = 0;
  	pthread_mutex_init(&ch->lock, NULL);
    maxID++;
    return ch;
}

void ch_push(chan* ch, void* item){
    pthread_mutex_lock(&ch->lock);
    if (ch->buf_head >= ch->cap){
        int thread_id;
        Pool* pool;
        match_registered_pool(&pool, &thread_id);
        /* pushing into a channel which is full */
        // this will wake any functions which are waiting to pull from this channel
        sched_pushing_full(pool->chan_scheduler, ch->id, thread_id, &ch->lock);
    }
    // add item to the buffer
    ch->buf_head++;
    (ch->buffer)[ch->buf_head] = item;
    pthread_mutex_unlock(&ch->lock);
    log(DEBUG_SCHED, "returning from push\n");
}

void* ch_pull(chan* ch){
    pthread_mutex_lock(&ch->lock);
    if (ch->buf_head == -1){
        int thread_id;
        Pool* pool;
        match_registered_pool(&pool, &thread_id);
        /* pulling from a channel which is empty */
        // this will wake any functions which are waiting to push into this channel
        sched_pulling_empty(pool->chan_scheduler, ch->id, thread_id, &ch->lock);
    }
    // pull from the buffer
    void* result = (ch->buffer)[ch->pull_head];
    ch->pull_head = ++ch->pull_head > ch->cap ? 0 : ch->pull_head;
    ch->buf_head--;
    pthread_mutex_unlock(&ch->lock);
    return result;
}

// close a channel
void ch_close(chan* ch){
    pthread_mutex_lock(&ch->lock);
    int i;
    for(i=0; i<ch->cap; i++){
        free(ch->buffer[i]);
    }
    free(ch->buffer);
    pthread_mutex_unlock(&ch->lock);
    log(DEBUG_SCHED, "returning from push\n");
};


