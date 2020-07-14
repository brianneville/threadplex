### contents
1. **hash** - implementation of a hashtable.
2. **logging** - header file included to allow logging to be done based on certain debug flags.
3. **sched** - the scheduler acts as an abstraction layer between the channel and the thread pool.
Each thread pool has its own scheduler, responsible for scheduling functions on and between threads.
``` 
|-----------|         |-----------|       |-------------|
|  channel  |  <===>  | scheduler | <===> | thread pool |
|-----------|         |-----------|       |-------------|
```
4. **threadpool and xorLL** - implementation of a thread pool using an XOR linkedlist    
(a slightly modified version of my old project [here](https://github.com/brianneville/c_threadpool))
