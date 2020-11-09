#include <stdio.h>
#include <stdlib.h>
#include "../libs/thread_pool/thread_pool.h"
#include "../libs/queue/queue.h"

typedef struct task_arg{
    int a;
}Task_Arg;

void task_func(void *arg){
    Task_Arg *argp = (Task_Arg *)(arg);
    printf("%d\n", argp->a);
}

int main(int argc, char** argv){
    ThreadPool *pool = create_thread_pool();
    Task_Arg ta;
    ta.a = 0;
    submit((void (*)(void *))task_func, (void *)(&ta), pool);
    return 0;
}