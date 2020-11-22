#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <omp.h>
#include <emmintrin.h>

#define DISINF 6000*1000+1
#define FULL 2147483647
#define DISZSELF 0
#define SIZEOFINT sizeof(int)
#define VECGAP 4
#define VECSCALE 2

int cpu_num = 0;
int vertex_num = 0, edge_num = 0, graph_size = 0;
int *buf = NULL;
int *graph = NULL;
int chunk_size = 8;

const int zero_vec[VECGAP] = {0};
const int one_vec[VECGAP] = {1, 1, 1, 1};
const unsigned int full_vec[VECGAP] = {FULL, FULL, FULL, FULL};
const __m128i zero_v = _mm_loadu_si128((const __m128i*)zero_vec);
const __m128i one_v = _mm_loadu_si128((const __m128i*)one_vec);
const __m128i full_v = _mm_loadu_si128((const __m128i*)full_vec);

void show_mat(int *g, int n){
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            printf("%d\t", g[i * n + j]);
        }
        printf("\n");
    }
}

void show_m128i(__m128i *m){
    printf("(%d\t%d\t%d\t%d)\n", ((int*)m)[0], ((int*)m)[1], ((int*)m)[2], ((int*)m)[3]);
}
int* get_graph_addr(int row, int col){return &(graph[row*vertex_num + col]);}
int get_graph(int row, int col){return graph[row*vertex_num + col];}
void set_graph(int row, int col, int val){graph[row*vertex_num + col] = val;}

void graph_malloc();
void buf2graph(int *, int *, int, int, int);
void omp_buf2graph(int *, int *, int, int, int);
void relax_v(int*, int*, int*);
void relax(int, int, int*);
void floyd_warshal();

typedef struct{
    pthread_t *threads;
    pthread_mutex_t *sync_lock;
    pthread_cond_t *sync_cond;
    int sync_counter;
    int threads_num;
    // int is_submit_done;
    int is_finish;
}ThreadPool;

typedef struct{
    ThreadPool *pool;
    int thread_id;
}WorkerArg;

ThreadPool *create_thread_pool(int);
void sync_threads(ThreadPool*);
void get_task();
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
    FILE *f_r = NULL, *f_w = NULL;
    f_r = fopen(argv[1], "r");
    f_w = fopen(argv[2], "w");
    assert(f_r != NULL);
    assert(f_w != NULL);

    fread(&vertex_num, SIZEOFINT, 1, f_r);
    fread(&edge_num, SIZEOFINT, 1, f_r);
    graph_size = vertex_num * vertex_num;
    buf = (int*)malloc(edge_num * SIZEOFINT * 3);
    fread(buf, SIZEOFINT, edge_num * 3, f_r);
    graph_malloc();
    
    printf("%d %d\n", vertex_num, edge_num);
    for(int i = 0; i < edge_num * 3; i += 3){
        printf("Edge %d - SRC: %d DST: %d WEIGHT: %d\n", i, buf[i], buf[i + 1], buf[i + 2]);
    }

    ThreadPool *pool = create_thread_pool(cpu_num);
    start_pool(pool);
    end_pool(pool);
    floyd_warshal();

    fwrite(graph, SIZEOFINT, graph_size, f_w);
    return 0;
}

void graph_malloc(){
    graph = (int*)malloc(graph_size * sizeof(int));
    memset(graph, DISZSELF, graph_size * sizeof(int));
}

void buf2graph(int *buf, int *graph, int start, int end, int gap){
    for(int i = start*3; i < end*3; i+=(gap*3)){
        printf("Func: Edge %d - SRC: %d DST: %d WEIGHT: %d\n", i, buf[i], buf[i + 1], buf[i + 2]);
        set_graph(buf[i], buf[i + 1], buf[i + 2]);
    }
    for(int idx = start; idx < graph_size; idx+=gap){
        int i = idx / vertex_num, j = idx % vertex_num;
        if(get_graph(i, j) <= 0 && i != j){
            set_graph(i, j, DISINF);
        }
    }
    // printf("pthread done\n");
}

// void omp_buf2graph(int *buf, int *graph, int start, int end, int gap){
//     #pragma omp parallel num_threads(cpu_num) shared(buf, graph, edge_num, vertex_num)
//     {
//         #pragma omp for schedule(static) nowait
//         for(int i = 0; i < edge_num*3; i+=3){
//             printf("Func: Edge %d - SRC: %d DST: %d WEIGHT: %d\n", i, buf[i], buf[i + 1], buf[i + 2]);
//             set_graph(buf[i], buf[i + 1], buf[i + 2]);
//         }
//     }
//     printf("---\n");
//     #pragma omp parallel num_threads(cpu_num)
//     {
//         #pragma omp for schedule(dynamic) nowait collapse(2)
//         for(int i = 0; i < vertex_num; i++){
//             for(int j = 0; j < vertex_num; j++){
//                 printf("Func: Edge - SRC: %d DST: %d WEIGHT\n", i, j);
//                 if(get_graph(i, j) <= 0 && i != j){
//                     printf("Func: Edge - SRC: %d DST: %d WEIGHT: INF\n", i, j);
//                     set_graph(i, j, DISINF);
//                 }
//             }
//         }
//     }
// }
// Relax with intermediate sequence k, from sequence i to j
void relax_v(int *aij, int aik, int *akj){
    show_mat(graph, vertex_num);
    show_m128i((__m128i*)aij);
    __m128i aij_v = _mm_loadu_si128((const __m128i*)aij);
    printf("aij_v:\n");
    show_m128i(&aij_v);
    const int aik_vec[VECGAP] = {aik, aik, aik, aik};
    __m128i aik_v = _mm_loadu_si128((const __m128i*)aik_vec);
    printf("aik_v:\n");
    show_m128i(&aik_v);
    __m128i akj_v = _mm_loadu_si128((const __m128i*)akj);
    printf("akj_v:\n");
    show_m128i(&akj_v);

    __m128i sum_v = _mm_add_epi32(aik_v, akj_v);
    printf("sum_v:\n");
    show_m128i(&sum_v);
    __m128i compare_gt_v = _mm_cmpgt_epi32(aij_v, sum_v);
    printf("compare_gt_v:\n");
    show_m128i(&compare_gt_v);
    __m128i compare_let_v = _mm_xor_si128(compare_gt_v, full_v);
    printf("compare_let_v:\n");
    show_m128i(&compare_let_v);

    __m128i compgt_sum = _mm_and_si128(compare_gt_v, sum_v);
    printf("compgt_sum:\n");
    show_m128i(&compgt_sum);
    __m128i complet_aij = _mm_and_si128(compare_let_v, aij_v);
    printf("complet_aij:\n");
    show_m128i(&complet_aij);
    __m128i res_v = _mm_or_si128(_mm_and_si128(compare_gt_v, sum_v), _mm_and_si128(compare_let_v, aij_v));
    printf("res_v:\n");
    show_m128i(&res_v);

    _mm_storeu_si128((__m128i*)aij, res_v);
    printf("AIJ: %d %d %d %d\n", aij[0], aij[1], aij[2], aij[3]);
    show_mat(graph, vertex_num);
}
// Relax with intermediate node k, from node i to j
void relax_s(int *aij, int aik, int akj){
    if((*aij) > aik + akj){
        (*aij) = aik + akj;
    }
}
// Relax the node from A(i,j) to A(i,j+size), ignore node which j+size > vertex_num
void relax(int idx, int ak, int size){
    int ai = idx / vertex_num, aj = idx % vertex_num;
    int i = ai, j = aj, remain_size = size;
    for(i = ai; i < vertex_num; i++){
        if(remain_size <= 0){return;}
        int truncated_size = j + remain_size > vertex_num? vertex_num - j : remain_size;
        int vec_size = (truncated_size >> VECSCALE) << VECSCALE;
        int vec_end = j + vec_size, single_end = j + truncated_size;
        remain_size -= truncated_size;
        
        // Relax with Vectorization speed up
        for(; j < vec_end; j+=VECGAP){
            relax_v(get_graph_addr(i, j), get_graph(i, ak), get_graph_addr(ak, j));
        }
        // Single relax
        for(; j < single_end; j++){
            relax_s(get_graph_addr(i, j), get_graph(i, ak), get_graph(ak, j));
        }
        j = 0;
    }
}

void floyd_warshal(){
    for(int k = 0; k < vertex_num; k++){
        // // Row dependency update
        // for(int j = 0; j < vertex_num; j++){
        //     // relax_v(get_graph_addr(k, j), get_graph(k, k), get_graph_addr(k, j));
        //     relax_s(get_graph_addr(k, j), get_graph(k, k), get_graph(k, j));
        // }
        // // Column dependency update
        // for(int i = 0; i < vertex_num; i++){
        //     relax_s(get_graph_addr(i, k), get_graph(k, k), get_graph(i, k));
        // }

        // // Inpdependent update
        // for(int i = 0; i < vertex_num; i++){
        //     for(int j = 0; j < vertex_num; j++){
        //         relax_s(get_graph_addr(i, j), get_graph(i, k), get_graph(k, j));
        //     }
        //     // relax_v(get_graph_addr(k, j), get_graph(k, k), get_graph_addr(k, j));
        // }

        // // Inpdependent update
        // for(int i = 0; i < vertex_num; i++){
        //     for(int j = 0; j + VECGAP < vertex_num; j+=VECGAP){
        //         printf("K: %d, I: %d, J: %d\n", k, i, j);
        //         relax_v(get_graph_addr(i, j), get_graph(i, k), get_graph_addr(k, j));
        //     }
        //     printf("Last\n");
        //     relax_s(get_graph_addr(i, 4), get_graph(i, k), get_graph(k, 4));
        // }

        // relax(0, k, 4);
        // relax(4, k, 3);
        // relax(7, k, 2);
        // relax(9, k, 1);
        // relax(10, k, graph_size - 10);

        // for(;;){
        //     int idx = 0, size = 0, is_next_k = 0;
        //     get_task(&idx, &size, &is_next_k);
        //     if(is_next_k){break;}
        //     relax(idx, k, size);
        // }

        #pragma omp parallel num_threads(cpu_num)
        {
            #pragma omp for schedule(guided) nowait
            for(int idx = 0; idx < vertex_num;){
                // relax(idx, k, size);
                // idx = idx + 4 > vertex_num? 
            }
        }
    }
}

ThreadPool *create_thread_pool(int threads_num){
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->threads = NULL;
    // pool->queue = queue;
    pool->sync_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pool->sync_cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pool->sync_counter = 0;
    pthread_mutex_init(pool->sync_lock, NULL);
    pthread_cond_init(pool->sync_cond, NULL);
    pool->threads_num = threads_num;
    pool->is_finish = 0;

    return pool;
}

int get_threads_num(ThreadPool* pool){
    return pool->threads_num;
}
void sync_threads(ThreadPool* pool){
    pthread_mutex_lock(pool->sync_lock);
    pool->sync_counter++;
    if(pool->sync_counter < pool->threads_num){
        pthread_cond_wait(pool->sync_cond, pool->sync_lock);
    }else{
        pthread_cond_signal(pool->sync_cond);
    }
    pthread_mutex_unlock(pool->sync_lock);
}
void get_task(int *idx, int *size, int *is_next_k){
    static int counter = 0;
    if(counter < graph_size){
        *idx = counter;
        if(counter + chunk_size <= graph_size){
            *size = chunk_size;
        }else{
            *size = graph_size - counter;
        }
        counter += (*size);
        *is_next_k = 0;
    }else{
        idx = 0;
        *is_next_k = 1;
    }
}

void *worker(void *arg){
    int thread_id = ((WorkerArg*)arg)->thread_id;
    ThreadPool *pool = ((WorkerArg*)arg)->pool;
    // printf("Created Thread %d\n", thread_id);

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