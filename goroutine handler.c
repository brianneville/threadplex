#include "chan.h"
#include "sched.h"
#include <pthread.h>

// // inter function message passing
// void over_reach(chan* ch);

static int maxID = 0;

// value of zero will make channel unbuffered
chan* ch_make(int buffer){
    //intialise buffer
    chan* ch = (chan*)malloc(sizeof(chan));
    ch->cap = buffer;
    ch->buffer = (void*)malloc((1+ch->cap)*sizeof(void));
    ch->id = maxID;
    ch->buf_head = -1;
  	pthread_mutex_init(&ch->lock, NULL);
    init_xLinkedList(&ch->inputCalls);
    init_xLinkedList(&ch->outputCalls);  

    maxID++;
}

void ch_in(chan* ch, void* entry){
    // pushing into a channel which is full
    if (ch->buf_head >= ch->cap){
        // goroutine blocks, add this to the input queue
    }
    pthread_mutex_lock(&ch->lock);
    

    pthread_mutex_unlock(&ch->lock);
}

void* ch_out(chan* ch){
    if (ch->buf_head = -1){
        // goroutine blocks, add this request to the output queue
        // pull this once 
    }
}

// close a channel
void close(chan* ch);


