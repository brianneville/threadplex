/**
  User-level threads example.
   Threads created using UNIX "ucontext" functions.
*/
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h> /* for makecontext/swapcontext routines */

/* Tiny little user-level thread library */
typedef void (*threadFn)(void);

/* Create a new user-level thread to run this function */
void makeThread(ucontext_t *T,threadFn fn) {
	getcontext(T);
	/* Allocate a stack for the new thread */
	int stackLen=32*1024;
	char *stack=(char*)malloc(sizeof(char) * stackLen);
	printf("New thread stack is at %p\n",stack);
	T->uc_stack.ss_sp=stack;
	T->uc_stack.ss_size=stackLen;
	T->uc_stack.ss_flags=0;
	makecontext(T,fn,0);
}

/* Context structures for two user-level threads. */
ucontext_t A,B;
enum {runTimes=20, delayLen=1*1000*1000};
int total=0;

void runA(void) {
	int somelocal;
	printf("Thread A: stack at %p\n",&somelocal);
	for (int n=0;n<runTimes;n++) {
		for (int i=0;i<delayLen;i++) {}
		printf("A (it %d)  (total %d)\n",n,total);
		total++;
		swapcontext(&A,&B); /* switch from thread A to thread B */
	}
	printf("Thread A finished.  Exiting program.\n");
	exit(1);
}

void runB(void) {
	int somelocal;
	printf("Thread B: stack at %p\n",&somelocal);
	for (int n=0;n<runTimes;n++) {
		for (int i=0;i<delayLen;i++) {}
		printf("     B (it %d)  (total %d)\n",n,total);
		total++;
		swapcontext(&B,&A); /* switch from thread B to thread A */
	}
}

long foo() {
	makeThread(&A,runA);
	makeThread(&B,runB);
	setcontext(&A); /* Start running thread A first */
	return 0;
}
int main(){
    return (int)foo();
}