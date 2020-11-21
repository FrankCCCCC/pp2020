#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <emmintrin.h>

#define DISINF 2147483647
#define DISZSELF 0
#define SIZEOFINT sizeof(int)
#define VECGAP 4

int cpu_num = 0;
int vertex_num = 0, edge_num = 0;
int *buf = NULL;
int *graph = NULL;

const int zero_vec[VECGAP] = {0};
const int one_vec[VECGAP] = {1, 1, 1, 1};
// const int two_vec[VECGAP] = {2, 2, 2, 2};
const __m128i zero_v = _mm_load_si128((const __m128i*)zero_vec);
const __m128i one_v = _mm_load_si128((const __m128i*)one_vec);

int get_graph(int row, int col){return graph[row*vertex_num + col];}
void set_graph(int row, int col, int val){graph[row*vertex_num + col] = val;}

// void graph_malloc_sig(int*, int);
void graph_malloc();
void buf2graph_sig(int, int, int, int**);
void buf2graph(int *, int **, int, int, int);
void relax_v(int*, int*, int*);
void relax(int, int, int*);
void floyd_warshal();

typedef struct{
    pthread_t *threads;
    pthread_mutex_t *lock;
    int threads_num;
    // int is_submit_done;
    int is_finish;
}ThreadPool;

typedef struct{
    ThreadPool *pool;
    int thread_id;
}WorkerArg;

ThreadPool *create_thread_pool(int);
int get_num_tasks(ThreadPool*);
int is_task_queue_empty(ThreadPool*);
void *worker(void*);
void start_pool(ThreadPool*);
void set_finish(ThreadPool *);
void end_pool(ThreadPool*);

int main(int argc, char** argv) {
    cpu_set_t cpu_set;
    sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
    cpu_num = CPU_COUNT(&cpu_set);
    printf("%d cpus available\n", cpu_num);

    assert(argc == 3);
    FILE *f = NULL;
    f = fopen(argv[1], "r");
    assert(f != NULL);

    fread(&vertex_num, SIZEOFINT, 1, f);
    fread(&edge_num, SIZEOFINT, 1, f);
    buf = (int*)malloc(edge_num * SIZEOFINT * 3);
    fread(buf, SIZEOFINT, edge_num * 3, f);
    graph_malloc();
    
    printf("%d %d\n", vertex_num, edge_num);
    for(int i = 0; i < edge_num * 3; i += 3){
        printf("Edge %d - SRC: %d DST: %d WEIGHT: %d\n", i, buf[i], buf[i + 1], buf[i + 2]);
    }

    ThreadPool *pool = create_thread_pool(cpu_num);
    start_pool(pool);
    end_pool(pool);


    return 0;
}

// void graph_malloc_sig(int *graph_row, int size){
//     graph_row = (int*)malloc(size);
// }
void graph_malloc(){
    // for(int i = start; i < end; i+=gap){
    //     graph_malloc_sig(graph[i], mem_size);
    // }
    graph = (int*)malloc(vertex_num * vertex_num * sizeof(int));
}
// void buf2graph_sig(int src, int dst, int weight, int *graph){
//     set_graph(src, col, weight) = weight;
// }

void buf2graph(int *buf, int *graph, int start, int end, int gap){
    for(int i = start*3; i < end*3; i+=(gap*3)){
        // buf2graph_sig(buf[i], buf[i + 1], buf[i + 2], graph);
        // printf("buf2graph: i: %d, gap: %d\n", i, gap);
        // printf("Func: Edge %d - SRC: %d DST: %d WEIGHT: %d\n", i, buf[i], buf[i + 1], buf[i + 2]);
        set_graph(buf[i], buf[i + 1], buf[i + 2]);
    }
}

void relax_v(int *aij, int aik, int *akj){
    __m128i aij_v = _mm_load_si128((const __m128i*)aij);
    __m128i akj_v = _mm_load_si128((const __m128i*)akj);
    int aik_vec[VECGAP] = {aik, aik, aik, aik};
    __m128i aik_v = _mm_load_si128((const __m128i*)aik_vec);

    __m128i sum_v = _mm_add_epi32(aik_v, akj_v);
    __m128i compare_gt_v = _mm_cmpgt_epi32(aij_v, sum_v);
    __m128i compare_let_v = _mm_xor_si128(compare_gt_v, one_v);
    //XOR _mm_xor_si128()

    __m128i res_v = _mm_or_si128(_mm_and_si128(compare_gt_v, sum_v), _mm_and_si128(compare_let_v, aij_v));

    _mm_store_si128((__m128i*)aij, res_v);
}

void relax(int *aij, int aik, int akj){
    if((*aij) > aik + akj){
        (*aij) = aik + akj;
    }
}

ThreadPool *create_thread_pool(int threads_num){
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = NULL;
    // pool->queue = queue;
    pool->lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pool->lock, NULL);
    pool->threads_num = threads_num;
    pool->is_finish = 0;

    return pool;
}

int get_threads_num(ThreadPool* pool){
    return pool->threads_num;
}
// int get_num_tasks(ThreadPool* pool){
//     return size(pool->queue);
// }
// int is_task_queue_empty(ThreadPool* pool){
//     return is_empty(pool->queue);
// }

void *worker(void *arg){
    int thread_id = ((WorkerArg*)arg)->thread_id;
    ThreadPool *pool = ((WorkerArg*)arg)->pool;
    // printf("Created Thread %d\n", thread_id);

    // graph_malloc(graph, 0, vertex_num, cpu_num, SIZEOFINT * vertex_num);
    buf2graph(buf, graph, thread_id, edge_num, pool->threads_num);


    pthread_exit(NULL);
}

void start_pool(ThreadPool* pool){
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * get_threads_num(pool));
    WorkerArg *worker_args = (WorkerArg*)malloc(sizeof(WorkerArg) * get_threads_num(pool));
    for(int i = 0; i < get_threads_num(pool); i++){
        worker_args[i].pool = pool;
        worker_args[i].thread_id = i;
        pthread_create(&(pool->threads[i]), NULL, worker, (void*)(&(worker_args[i])));
    }
}

void set_finish(ThreadPool *pool){
    pool->is_finish = 1;
}

// Join the threads
void end_pool(ThreadPool* pool){
    pool->is_finish = 1;
    for(int i = 0; i < get_threads_num(pool); i++){
        pthread_join(pool->threads[i], NULL);
    }
}