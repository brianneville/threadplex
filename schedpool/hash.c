#include "hash.h"

int hashfunc_modkey(hashtable* h, int key){
    const int table_size = h->table_size;
    return key >= table_size ? key % table_size : key;
}

int hashfunc_sumpointer(hashtable* h, int key){
    int hash=0;
    unsigned int k = key * 0x9E3779B9;
    while(k){
        hash += k & 0xf;
        k >>= 8;
    }
    return hash % h->table_size;
}

hashtable* hashtable_create(int table_size, int use_chaining, hashfunc hashf){
    hashtable* h = (hashtable*)malloc(sizeof(hashtable));
    h->table_size = table_size;
    h->table = (node**)calloc(table_size, sizeof(node*));
    h->use_chaining = use_chaining;
    h->hashfunc = hashf;
    pthread_mutex_init(&(h->hashlock), NULL);
    return h;
}

void hashtable_print(hashtable* h){
    int i;
    for (i=0; i<h->table_size; i++){
        node* head = h->table[i];
        printf("head %d: %p", i, head);
        while(head){
            printf("[key %ld val %p next %p]", head->key, head->val, head->next);
            head = head->next;
        }
        printf("\n");
    }
}

// hashtable_insert will append duplicates onto the same linkedlist
void hashtable_insert(hashtable* h, unsigned long key, void* val){
    int indx = h->hashfunc(h, key);
    pthread_mutex_lock(&(h->hashlock));
    node** headptr = &(h->table[indx]);
    node* new_entry = (node*)malloc(sizeof(node));
    new_entry->next = NULL;
    new_entry->val = val;
    new_entry->key = key;
    if(!*headptr || !(h->use_chaining)){
        *headptr = new_entry;
    }else{
       while((*headptr)->next){
            headptr = &((*headptr)->next);
        }
        (*headptr)->next = new_entry;
    }
    pthread_mutex_unlock(&(h->hashlock));
}

static node* hashtable_remove_internal(hashtable* h, unsigned long key, int useval, void* val){
    int indx = h->hashfunc(h, key);
    pthread_mutex_lock(&(h->hashlock));
    node** headptr = &(h->table[indx]);
    while(*headptr != NULL && (*headptr)->key != key &&  (*headptr)->val != val){
        findanother: headptr = &(*headptr)->next;
    }
    if(*headptr && (*headptr)->val != val && useval)goto findanother;
    node* out = *headptr;
    // "i can do this with the comma operator"
    return (!out) ? (pthread_mutex_unlock(&(h->hashlock)), NULL) :
    (free(*headptr), *headptr = (out)->next, pthread_mutex_unlock(&(h->hashlock)), out);
}

node* hashtable_remove(hashtable* h, unsigned long key){
    return hashtable_remove_internal(h, key, 0, NULL);
}

node* hashtable_removewithval(hashtable* h, unsigned long key, void* val){
    return hashtable_remove_internal(h, key, 1, val);
}

static node* hashtable_find_internal(hashtable* h, unsigned long key, int useval, void* val){
    int indx = h->hashfunc(h, key);
    node* head = h->table[indx];
    while(head && head->key != key){
        findanother: head = head->next;
    }
    if(head && head->val != val && useval)goto findanother;
    return head;
}


// hashtable_find returns the item if found, NULL if not present
node* hashtable_find(hashtable* h, unsigned long key){
    return hashtable_find_internal(h, key, 0, NULL);
}

node* hashtable_findwithval(hashtable* h, unsigned long key, void* val){
    return hashtable_find_internal(h, key, 1, val);
}

void hashtable_cleanup(hashtable* h){
    int i;
    node* prev;
    for (i=0; i<h->table_size; i++){
        node* head = h->table[i];
        while(head != NULL){
            prev = head;
            head = head->next;
            free(prev);
        }
    }
    free(h->table);
}

/*
static void test(){
    hashtable* h;
    const int table_size = 10;
    h = hashtable_create(table_size, USE_CHAINING, hashfunc_modkey);
    hashtable_insert(h, 2, (void*)1);
    hashtable_insert(h, 12, (void*)2);
    hashtable_insert(h, 22, (void*)3);
    hashtable_insert(h, 12, (void*)4);
    hashtable_insert(h, 0, (void*)5);
    hashtable_print(h);
    void* out;
    out = hashtable_removewithval(h, 12, (void*)111);
    printf("out is %p\n", out);
    hashtable_print(h);
    out = hashtable_remove(h, 12);
    printf("out is %p\n", out);
    hashtable_print(h);
    printf("item found: %p\n",hashtable_find(h, 9));
    printf("item found: %p\n",hashtable_findwithval(h, 22, (void*)3));
    hashtable_cleanup(h);
    free(h);
}

int main(){
     test();
} */