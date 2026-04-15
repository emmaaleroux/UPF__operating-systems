/* Implementing a queue data structure helps you implement cooperative threads,
 * but this is optional and you can maintain arrays instead of queues.
 */

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>


typedef struct Node {
    void* data;
    struct Node *next;
} Node;

struct queue {
    /* Student's code goes here (Cooperative Threads). */
    Node *front;
    Node *rear;
    /* Student's code ends here. */
};
typedef struct queue * queue_t;

queue_t queue_new() {
    queue_t queue      = malloc(sizeof(*queue));
    /* Student's code goes here (Cooperative Threads). */
    queue->front = NULL;
    queue->rear = NULL;
    /* Student's code ends here. */
    return queue;
}

int queue_enqueue(queue_t queue, void* item) {
    Node* p = malloc(sizeof(Node));
    if (p == NULL)
        return -1;
    p->next = NULL;
    p->data = item;

    /* Student's code goes here (Cooperative Threads). */
    if (queue->front == NULL) {
        queue->front = p;
        queue->rear = p;
    }
    else{
        queue->rear->next = p;
        queue->rear = p;
    }
    /* Student's code ends here. */
    return 0;
}

int queue_dequeue(queue_t queue, void** pitem) {
    /* Student's code goes here (Cooperative Threads). */
    if (queue->front == NULL) {
        return -1;
    }
    *pitem = queue->front->data;
    Node* old_front = queue->front;
    queue->front = queue->front->next;
    free(old_front);
    if (queue->front == NULL)
        queue->rear = NULL;

    /* Student's code ends here. */
    return 0;
}
