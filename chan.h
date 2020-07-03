#ifndef CHAN_H
#define CHAN_H

#ifndef DEBUG_CHAN
#define DEBUG_CHAN 0 
#endif

#include "xor_LL.h"

typedef struct chan{
    void* buffer;
    xLinkedList inputCalls;
    xLinkedList outputCalls;
    int buf_head;
    int cap;
    int id;
    pthread_mutex_t lock; // lock for in/out operations
}chan;

static int maxID = 0;

// ch_make creates a channel for data of type void*
// buffer specifies how many items this channel can allow
chan* ch_make(int buffer);

// ch_in adds a new item into the channel. 
// and adding an entry to a queue
void ch_in(chan* ch, void* entry);

// ch_out pulls an item from the channel and returns it to the caller
void* ch_out(chan* ch);

// close a channel
void close(chan* ch);


#endif
