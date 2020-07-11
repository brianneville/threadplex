#ifndef HASH_H
#define HASH_H

#ifndef DEBUG_HASH
#define DEBUG_HASH 0
#endif

#define USE_CHAINING 1
#define NO_CHAINING 0

#include <pthread.h>
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct node{
    unsigned long key;
    void* val;
    struct node* next;
}node;

typedef struct hashtable{
    node** table;
    pthread_mutex_t hashlock;
    int table_size;
    int use_chaining;
}hashtable;

// create a new hashtable with table_size slots of type node*. collisions are chained.
hashtable* hashtable_create(int table_size, int use_chaining);

//insert into hashtable
void hashtable_insert(hashtable* h, unsigned long key, void* val);

// find node by key
node* hashtable_find(hashtable* h, unsigned long key);

//remove item from hashtable
void* hashtable_remove(hashtable* h, unsigned long key);

//print contents of hashtable
void hashtable_print(hashtable* h);

// cleanup linkedlist structure
void hashtable_cleanup(hashtable* h);

#endif
