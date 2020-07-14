#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 5
typedef struct sched{
    jmp_buf* env_store;
}sched;

void sched_init(sched** s){
    // store for envs at start of threadpool pulling
    *s = (sched*)malloc(sizeof(sched));
    (*s)->env_store = (jmp_buf*)malloc(POOL_SIZE*sizeof(jmp_buf));
}



int main()
{   
    sched* chan_scheduler;
    sched_init(&chan_scheduler);
    
    jmp_buf* env = (jmp_buf*)malloc(sizeof(jmp_buf));
    int i;
    i = setjmp(*env);
    if (i == 1){
		goto startpulling;
	}
	if (i == 2){
	    goto end;
	}
	jmp_buf** iter = &(chan_scheduler->env_store);
	*(iter+3*sizeof(jmp_buf))= env;

	printf("env is %p\n", env);

    jmp_buf** ptr;
    ptr = &(chan_scheduler->env_store);
    ptr = ptr + 3*sizeof(jmp_buf);
	
	printf("back env is %p\n", ptr);

	
	longjmp(*ptr[0], 1);
	return 0;
startpulling:
    printf("good 1\n");
    longjmp(*ptr[0], 2);
    
end:
    printf("good 2\n");
    return 0;
}