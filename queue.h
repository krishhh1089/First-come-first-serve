#ifndef QUEUE_H
#define QUEUE_H

#include "pcb.h"

/*
 * Queue management structure for process dispatching
 */
typedef struct {
    PcbPtr head;
    PcbPtr tail;
    int size;
} Queue;

/* Queue API */
void initQueue(Queue *q);
int isEmpty(const Queue *q);
void enqueue(Queue *q, PcbPtr p);
PcbPtr dequeue(Queue *q);
PcbPtr peek(const Queue *q);
void printQueue(const Queue *q);
void freeQueue(Queue *q);

/* Linked-list helper functions matching classic course API */
PcbPtr enqPcb(PcbPtr head, PcbPtr p);
PcbPtr deqPcb(PcbPtr *head);

#endif /* QUEUE_H */
