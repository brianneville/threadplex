#include "sched.h"
#include <stdlib.h>
#include <search.h>

void suspend(sched* s){
    s->suspendedCount++;
    realloc(s->suspendedContexts, s->suspendedCount * sizeof(ucontext_t));
    ucontext_t* ctx;
    getcontext(ctx);

}

void unsuspend(sched* s){
    s->suspendedCount--;
    realloc(s->suspendedContexts, s->suspendedCount * sizeof(ucontext_t));
    // ucontext_t* ctx = 
    // getcontext(ctx);

}

void sched_init(sched* s){
    ENTRY e, *ep;
    int i;
    hcreate(TABLE_SIZE);
}

void sched_destroy(sched* s){}
