#include "hash.h"

static int hash(hashtable* h, int key){
    return key % h->table_size;
}

hashtable* hashtable_create(int table_size, int use_chaining){
    hashtable* h = (hashtable*)malloc(sizeof(hashtable));
    h->table_size = table_size;
    h->table = (node**)calloc(table_size, sizeof(node*));
    h->use_chaining = use_chaining;
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
// hashtable_insert will append duplicates onto the same linkedlist, and 
// hashtable_remove will remove duplicates in the order that they were put in.
// this means that multiple goroutines can all talk to the same channel, and the
// order which these messages are pulled from the queue will be logical?
void hashtable_insert(hashtable* h, unsigned long key, void* val){
    int indx = hash(h, key);
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

void* hashtable_remove(hashtable* h, unsigned long key){
    int indx = hash(h, key);
    pthread_mutex_lock(&(h->hashlock));
    node** headptr = &(h->table[indx]);
    while(*headptr != NULL && (*headptr)->key != key){
        headptr = &(*headptr)->next;
    }
    node* out = *headptr;
    // "i can do this with the comma operator"
    return (!out) ? (pthread_mutex_unlock(&(h->hashlock)), NULL) :
    (free(*headptr), *headptr = (out)->next, pthread_mutex_unlock(&(h->hashlock)), out->val);
}

// hashtable_find returns the item if found, NULL if not present
node* hashtable_find(hashtable* h, unsigned long key){
    int indx = hash(h, key);
    node* head = h->table[indx];
    while(head && head->key != key){
        head = head->next;
    }
    return head;
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

static void test(){
    hashtable* h;
    int table_size = 10;
    h = hashtable_create(table_size, NO_CHAINING);
    hashtable_insert(h, 2, (void*)1);
    hashtable_insert(h, 12, (void*)2);
    hashtable_insert(h, 22, (void*)3);
    hashtable_insert(h, 12, (void*)4);
    hashtable_insert(h, 0, (void*)5);
    hashtable_print(h);
    void* out;
    out = hashtable_remove(h, 12);
    printf("out is %p\n", out);
    hashtable_print(h);
    out = hashtable_remove(h, 12);
    printf("out is %p\n", out);
    hashtable_print(h);
    printf("item found: %p\n",hashtable_find(h, 9));
    printf("item found: %p\n",hashtable_find(h, 22));
    hashtable_cleanup(h);
    free(h);
}

// int main(){
//     test();
// }