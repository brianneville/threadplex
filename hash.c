#include "hash.h"

int hash(int key){
    return key % TABLE_SIZE;
}

hashtable hashtable_create(){
    hashtable* h = (hashtable*)malloc(sizeof(hashtable));
	pthread_mutex_init(&(h->hashlock), NULL);
    int i;
    for (i = 0; i < TABLE_SIZE; i ++){
        h->table[i] = NULL;
    }
    return *h;
}

void hashtable_print(hashtable* h){
    int i;
    for (i=0; i<TABLE_SIZE; i++){
        node* head = h->table[i];
        printf("head: %p", head);
        while(head != NULL){
            printf("[key %d ctx %p next %p]", head->key, head->ctx, head->next);
            head = head->next;
        }
        printf("\n");
    }
}
// hashtable_insert will append duplicates onto the same linkedlist, and 
// hashtable_remove will remove duplicates in the order that they were put in.
// this means that multiple goroutines can all talk to the same channel, and the
// order which these messages are pulled from the queue will be logical?
void hashtable_insert(hashtable* h, uint key, ucontext_t* ctx){
    int indx = hash(key);
    pthread_mutex_lock(&(h->hashlock));
    node** head = &(h->table[indx]);
    node* new_entry = (node*)malloc(sizeof(node));
    new_entry->next = NULL;
    new_entry->ctx = ctx;
    new_entry->key = key;
    if(*head == NULL){
        *head = new_entry;
    }else{
       while((*head)->next != NULL){
            head = &((*head)->next);
        }
        (*head)->next = new_entry;
    }
    pthread_mutex_unlock(&(h->hashlock));
}

ucontext_t* hashtable_remove(hashtable* h, uint key){
    int indx = hash(key);
    pthread_mutex_lock(&(h->hashlock));
    node** headptr = &(h->table[indx]);
    while((*headptr)->key != key && headptr != NULL){
        headptr = &(*headptr)->next;
    }
    node* out = *headptr;
    *headptr = (*headptr)->next;
    pthread_mutex_unlock(&(h->hashlock));
    if(out == NULL)return NULL;
    return out->ctx;
}


void test(){
    hashtable h;
    h = hashtable_create();
    hashtable_insert(&h, 2, NULL);
    hashtable_insert(&h, 12, 0x1);
    hashtable_insert(&h, 22, NULL);
    hashtable_insert(&h, 12, 0x2);
    hashtable_insert(&h, 0, NULL);
    hashtable_print(&h);
    ucontext_t* uc;
    uc = hashtable_remove(&h, 12);
    printf("uc is %p\n", uc);
    hashtable_print(&h);
    uc = hashtable_remove(&h, 12);
    printf("uc is %p\n", uc);
    hashtable_print(&h);
}

// int main(){
//     test();
// }