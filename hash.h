#ifndef HASH_H
#define HASH_H

#ifndef DEBUG_HASH
#define DEBUG_HASH 0
#endif

#ifndef TABLE_SIZE
#define TABLE_SIZE 10
#endif

#include <pthread.h>
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

int hash(int key);

typedef struct node{
    uint key;   // default value of 
    ucontext_t* ctx;
    struct node* next;
}node;

typedef struct hashtable{
    node* table[TABLE_SIZE];
    pthread_mutex_t hashlock;
}hashtable;

// create a new hashtable, an array of size TABLE_SIZE, of type node*
// each entry is a linkedlist of ucontext_t structs that are saved for a channel
hashtable hashtable_create();

//insert into linkedlist 
void hashtable_insert(hashtable* h, uint key, ucontext_t* ctx);

//remove item from linkedlist
ucontext_t* hashtable_remove(hashtable* h, uint key);

//print contents of hashtable
void hashtable_print(hashtable* h);

#endif
