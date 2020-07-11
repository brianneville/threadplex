#ifndef CHAN_H
#define CHAN_H

#ifndef DEBUG_CHAN
#define DEBUG_CHAN 0 
#endif

#ifndef IDCACHE_SIZE
#define IDCACHE_SIZE 10
#endif

#include "schedpool/threadpool.h"

typedef struct chan{
    void** buffer;
    int buf_head;
    int cap;
    int id;
    pthread_mutex_t lock; // lock for in/out operations
}chan;

static volatile int maxID = 0;

// ch_make creates a channel for data of type void*
// buffer specifies how many items this channel can allow
chan* ch_make(int buffer);

// ch_push adds a new item into the channel. 
// and adding an item to a queue
void ch_push(chan* ch, void* item);

// ch_pull pulls an item from the channel and returns it to the caller
void* ch_pull(chan* ch);

// close a channel
void ch_close(chan* ch);


#endif
