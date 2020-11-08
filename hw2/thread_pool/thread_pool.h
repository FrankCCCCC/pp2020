#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>

typedef struct task{
    void (*func)(void *);
    void *args;
}Task;

typedef struct{
    pthread_t *threads;
    
    int threads_num;
    int queue_size;
    int is_finish;
}ThreadPool;

ThreadPool *create_thread_pool();
void submit(void (*func)(void *), void *, ThreadPool *);
void worker(Task *);
void start_pool(ThreadPool *);
void end_pool(ThreadPool *);
