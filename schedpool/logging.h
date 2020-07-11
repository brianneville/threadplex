#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>

#define THROWAWAY         1

#define DEBUG_REGISTRY    0
#define DEBUG_THREADPOOL  0
#define DEBUG_XOR_LL      0
#define DEBUG_SCHED       1
#define DEBUG_CHAN        0

#define execute_logger(tagname, dodebug, ...) \
    do{if(dodebug)printf("\033[36m" tagname " : \033[0m" __VA_ARGS__);}while(0)

#define execute_err_logger(tagname, dodebug, ...) \
    do{if(dodebug)fprintf(stderr,"\033[36m" tagname " : \033[0m" __VA_ARGS__);}while(0)

// log with printf to stdout
#define log(tag, ...) execute_logger(#tag, tag, __VA_ARGS__)

// log with fprintf to stder
#define flog(tag, ...) execute_err_logger(#tag, tag, __VA_ARGS__)

#endif 