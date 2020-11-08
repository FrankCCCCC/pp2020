#include <stdio.h>
#include <stdlib.h>
#include "../thread_pool/thread_pool.h"

typedef struct task_arg{
    int a;
}Task_Arg;

void task_func(void *arg){
    Task_Arg *argp = (Task_Arg *)(arg);
    printf("%d\n", argp->a);
}

int main(int argc, char** argv){
    Task_Arg ta;
    ta.a = 0;
    submit((void (*)(void *))task_func, (void *)(&ta));
    return 0;
}