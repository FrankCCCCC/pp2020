// Doubly Linked List Queue
// #include <stdio.h>
#include <stdlib.h>

typedef struct node{
    struct node *next;
    struct node *prev;
    void *data;
}Node;

typedef struct queue{
    Node *head;
    Node *tail;
    int size;
}Queue;

Queue *create_queue();
void push(Queue *, void *);
Node *pop(Queue *);
int is_empty(Queue *);
