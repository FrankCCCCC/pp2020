#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../libs/thread_pool/thread_pool.h"
// #include "../libs/queue/queue.h"

typedef struct task_arg{
    int a;
    int res;
}Task_Arg;

void hard_test_queue(){
    printf("Start Hard Queue Test...\n");
    Queue *q = create_queue();
    int testcase_num = 10;
    assert(is_q_empty(q));
    assert(get_q_size(q) == 0);
    for(int i = 0; i < testcase_num; i++){
        assert(get_q_size(q) == i);
        Task_Arg *t_p = (Task_Arg*)malloc(sizeof(Task_Arg));
        t_p->a = i;
        printf("Push: %d\n", t_p->a);

        push(q, (void *)(t_p));
        printf("Head: %d, Tail: %d, Size: %d\n", 
            ((Task_Arg *)(q->head->data))->a, 
            ((Task_Arg *)(q->tail->next->data))->a, 
            get_q_size(q));
        assert(!is_q_empty(q));
        
    }
    for(int i = 0; i < testcase_num/2; i++){
        assert(get_q_size(q) == testcase_num - i);
        assert(!is_q_empty(q));
        Task_Arg *t_p = (Task_Arg*)pop(q);
        printf("Poped: %d - Correct: %d\n", t_p->a, i);
        assert(t_p->a == i);
        
        free(t_p);
    }
    for(int i = testcase_num/2; i < testcase_num; i++){
        assert(get_q_size(q) == i);
        Task_Arg *t_p = (Task_Arg*)malloc(sizeof(Task_Arg));
        t_p->a = i;
        printf("Push: %d\n", t_p->a);

        push(q, (void *)(t_p));
        printf("Head: %d, Tail: %d, Size: %d\n", 
            ((Task_Arg *)(q->head->data))->a, 
            ((Task_Arg *)(q->tail->next->data))->a, 
            get_q_size(q));
        assert(!is_q_empty(q));
    }
    for(int i = 0; i < testcase_num/2; i++){
        assert(get_q_size(q) == testcase_num - i);
        assert(!is_q_empty(q));
        Task_Arg *t_p = (Task_Arg*)pop(q);
        printf("Poped: %d - Correct: %d\n", t_p->a, i + testcase_num/2);
        assert(t_p->a == (i + testcase_num/2));
        
        free(t_p);
    }
    for(int i = 0; i < testcase_num/2; i++){
        assert(get_q_size(q) == testcase_num/2 - i);
        assert(!is_q_empty(q));
        Task_Arg *t_p = (Task_Arg*)pop(q);
        printf("Poped: %d - Correct: %d\n", t_p->a, i + testcase_num/2);
        assert(t_p->a == (i + testcase_num/2));
        
        free(t_p);
    }
    assert(is_q_empty(q));
}

void easy_test_queue(){
    printf("Start Easy Queue Test...\n");
    Queue *q = create_queue();
    int testcase_num = 10;
    assert(is_q_empty(q));
    assert(get_q_size(q) == 0);
    for(int i = 0; i < testcase_num; i++){
        assert(get_q_size(q) == i);
        Task_Arg *t_p = (Task_Arg*)malloc(sizeof(Task_Arg));
        t_p->a = i;
        printf("Push: %d\n", t_p->a);

        push(q, (void *)(t_p));
        printf("Head: %d, Tail: %d, Size: %d\n", 
            ((Task_Arg *)(q->head->data))->a, 
            ((Task_Arg *)(q->tail->next->data))->a, 
            get_q_size(q));
        assert(!is_q_empty(q));
        
    }
    for(int i = 0; i < testcase_num; i++){
        assert(get_q_size(q) == testcase_num - i);
        assert(!is_q_empty(q));
        Task_Arg *t_p = (Task_Arg*)pop(q);
        printf("Poped: %d - Correct: %d\n", t_p->a, i);
        assert(t_p->a == i);
        
        free(t_p);
    }
    assert(is_q_empty(q));
}

void task_func(void *arg){
    Task_Arg *argp = (Task_Arg *)(arg);
    // printf("Thread %d: %d\n", pthread_self(), argp->a);
    argp->res++;
}

void test_thread_pool(){
    ThreadPool *pool = create_thread_pool(50);
    int task_num = 10000000;
    Task_Arg *tas = (Task_Arg*)malloc(sizeof(Task_Arg) * task_num);
    for(int i = 0; i < task_num/5; i++){
        tas[i].a = i;
        tas[i].res = 0;
        submit((void (*)(void *))task_func, (void *)(&(tas[i])), pool);
    }
    start_pool(pool);
    for(int i = task_num/5; i < task_num; i++){
        tas[i].a = i;
        tas[i].res = 0;
        submit((void (*)(void *))task_func, (void *)(&(tas[i])), pool);
    }
    submit_done(pool);
    end_pool(pool);

    int sum = 0;
    for(int i = 0; i < task_num; i++){
        assert(tas[i].res == 1);
        sum += tas[i].res;
    }
    assert(sum == task_num);
    printf("Sum: %d - Correct: %d\n", sum, task_num);
}

int main(int argc, char** argv){
    // Queue Testing
    // hard_test_queue();
    // easy_test_queue();

    // ThreadPool Testing
    test_thread_pool();

    return 0;
}