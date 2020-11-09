#include "thread_pool.h"

ThreadPool *create_thread_pool(int threads_num){
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = NULL;
    pool->queue = create_queue();
    pool->lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pool->lock, NULL);
    pool->threads_num = threads_num;
    pool->is_submit_done = 0;
    pool->is_finish = 0;

    return pool;
}

void set_threads_num(ThreadPool* pool, int threads_num){
    pool->threads_num = threads_num;
}
int get_threads_num(ThreadPool* pool){
    return pool->threads_num;
}
int get_num_tasks(ThreadPool* pool){
    return get_q_size(pool->queue);
}
int is_finish(ThreadPool* pool){
    return pool->is_finish;
}

void submit(void (*func)(void *), void *arg, ThreadPool *pool){
    // func(args);
    if(is_submit_done(pool)){return;} // Block task assignment after calling submit_done
    Task *new_task = (Task*)malloc(sizeof(Task));
    new_task->func = func;
    new_task->arg = arg;
    push(pool->queue, (void *)new_task);
}

void submit_done(ThreadPool* pool){
    pool->is_submit_done = 1;
}

int is_submit_done(ThreadPool* pool){
    return pool->is_submit_done;
}

Task *get_task(ThreadPool* pool){
    return (Task*)(pop(pool->queue));
}

void *worker(void *worker_arg_v){
    WorkerArg *worker_arg = (WorkerArg*)worker_arg_v;
    ThreadPool *pool = worker_arg->pool;
    int thread_id = worker_arg->thread_id;
    printf("Thread %d Created(Self: %d)\n", thread_id, pthread_self());
    Task *task = NULL;

    for(;(!is_submit_done(pool)) || (get_num_tasks(pool) > 0);){
        pthread_mutex_lock(pool->lock);
        if(get_num_tasks(pool) > 0){
            task = get_task(pool);
            task->func(task->arg);
        }
        pthread_mutex_unlock(pool->lock);
    }

    pthread_exit(NULL);
}

// Create and start the threads
void start_pool(ThreadPool* pool){
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * get_threads_num(pool));
    WorkerArg *worker_args = (WorkerArg*)malloc(sizeof(WorkerArg) * get_threads_num(pool));
    for(int i = 0; i < get_threads_num(pool); i++){
        worker_args[i].pool = pool;
        worker_args[i].thread_id = i;
        pthread_create(&(pool->threads[i]), NULL, worker, (void*)(&(worker_args[i])));
    }
}

// Join the threads
void end_pool(ThreadPool* pool){
    for(int i = 0; i < get_threads_num(pool); i++){
        pthread_join(pool->threads[i], NULL);
    }
    pool->is_finish = 1;
}

// Reset the task queue, is_finish flag, and is_submit_done flag
void resest_pool(ThreadPool* pool){
    free(pool->queue);
    pool->is_finish = 0;
    pool->is_submit_done = 0;
}

// Free the whole ThreadPool object
void free_pool(ThreadPool* pool){
    free(pool->threads);
    free(pool->queue);
    free(pool->lock);
    free(pool);
}