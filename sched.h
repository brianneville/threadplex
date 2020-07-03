#ifndef SCHED_H
#define SCHED_H

#ifndef DEBUG_SCHED
#define DEBUG_SCHED 0
#endif

#define TABLE_SIZE 10

#include <ucontext.h>

typedef struct sched{
    ucontext_t* suspendedContexts;
    int suspendedCount;
}sched;

void sched_init(sched* s);


#endif
