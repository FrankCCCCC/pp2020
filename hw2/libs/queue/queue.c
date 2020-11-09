// Doubly Linked List Queue
#include "queue.h"

// While initializztion, head and tail points to dummy node.
Queue *create_queue(){
    Queue *q = (Queue*)malloc(sizeof(Queue));
    Node *dum = (Node*)malloc(sizeof(Node));
    dum->data = NULL;
    dum->next = NULL;
    dum->prev = NULL;

    q->tail = dum;
    q->head = dum;
    q->size = 0;
    return q;
}

// next represent the neighbor node toward head direction, 
// while prev is toward tail.
void push(Queue *q, void *data){
    Node *new_node = (Node*)malloc(sizeof(Node));
    Node *temp = q->tail->next;

    new_node->data = data;
    new_node->next = temp;
    new_node->prev = q->tail;

    q->tail->next = new_node;

    temp->prev = new_node;

    q->size++;

    if(is_empty(q)){
        q->head = new_node;
    }
}