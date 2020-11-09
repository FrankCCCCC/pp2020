#include "thread_pool.h"
// #include "queue.h"

void submit(void (*func)(void *), void *args, ThreadPool *pool){
    func(args);
}

