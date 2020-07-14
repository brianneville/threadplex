# threadplex

### goal

The goal here is to be able to allow channel operations (pushing items into a channel and withdrawing items from a channel) to work between functions that are pushed into a thread pool.


### design

Addionally where possible, blocking and unblocking functions should be done cooperatively, and threads should alert other threads to adjust their scheduling where needed.


### the problem 

This project is currently broken due to the fact that stack frames are unwound when doing a longjmp from `sched_executionredirect` to `pull_from_queue`, and therefore the env saved in `sched_executionredirect` cannot be resumed. 
longjmp implementation varies here, but the result is consistent and envs saved in stack frames on lower addresses cannot be safely returned to 😣😩


Ideally you could find some way to ensure that these stack frames will exist, and then rewind down to return to the saved env. Unfortunately from what I can tell, this is unsupported - although perhaps there's some kind of arcane Assembly that could pull it off.


#### example/explantion 

 as per the longjmp [docs](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/longjmp?view=vs-2019) :
 "Only call longjmp before the function that called setjmp returns; otherwise the results are
  unpredictable."
 when doing a longjmp, the stack is unwound to find the position that it should jump to. 
 stack view:
```
    top of stack thread A             :    top of stack thread B          
                                      :
 pull_from_queue <-------<-----<      :    pull_from_queue ------------v
                         |     |      :                                | 
 blocking function call -^     ^==trigger== unblocking function call <-<
                                      : 

1. Thread A runs and blocks
2. Thread B runs and unblocks the channel for A, and then retriggers A inside pull_from_queue
3. Segmentation fault (core dumped)
```
 When the unblocking function call retriggers the pull_from_queue in thread A, thread A tries to jump to an address which is lower down on the stack (ie. it tries to restart the previously blocked function).
 However, since this is lower in the stack than the pull_from_queue, the stack unwinding cannot find this address, and we get a segfault. 


#### further design comments

- ucontext_t offers the ability to save and restore stack context, could it be useful here?

Perhaps I could make this work, but it would be expensive and it would absolutely kill the coroutine-esque style of concurrency that I'm going for here. Using makecontext would essentially be like creating more threads to handle blocking operation. 

At that point, I may as well just spin up contexts (or new threads) for each function that get pushed into the thread pool, assign them all condition variables, and wake these threads whenever they become unblocked. 
Additionally, ucontext_t functions are not POSIX, and so should be avoided for compatability reasons.

- why I need both pending_envs *and* env_to_id?

`pull_from_queue` only has access to the thread_id, and needs to use this to find the env that it should jmp to - therefore to avoid o(n) search through all envs, it should use a map[id]env this is done with the `hashtable* pending_envs`.

Scheduler needs to know which thread_id an env belongs to, when given only this env. Therefore to avoid having to loop o(n) through all thread_ids in the map[id]env, it should use a map[env]id this is done with the `hashtable* env_to_id`.

- why isnt it all just coroutine-style jmp-ing on top of a thread pool?

setjmp/longjmp only recognise the stack within their own thread, and cannot jmp execution from one thread to another. This, combined with the fact that we have no knowledge about the structure of the functions in the threadpool (i.e. we cannot know which functions have channel operations, or when or where these operations are going to block), means that we must use cross-thread triggers (done here with the pending_env hashtable) to tell other threads that they should resume a function.