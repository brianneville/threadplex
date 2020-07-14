SCHEDPOOL = schedpool/threadpool.c schedpool/sched.c schedpool/hash.c  schedpool/xor_LL.c
FILES = vision.c chan.c $(SCHEDPOOL)

basic: $(FILES)
	gcc -Wall $(FILES) -o vision -lpthread

