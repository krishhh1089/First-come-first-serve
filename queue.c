#include "queue.h"

/*
 * Initialize queue structure
 */
void initQueue(Queue *q) {
    if (!q) return;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

/*
 * Check if the queue is empty
 */
int isEmpty(const Queue *q) {
    return (!q || q->head == NULL);
}

/*
 * Enqueue a PCB to the tail of the queue
 */
void enqueue(Queue *q, PcbPtr p) {
    if (!q || !p) return;
    p->next = NULL;

    if (q->head == NULL) {
        q->head = p;
        q->tail = p;
    } else {
        q->tail->next = p;
        q->tail = p;
    }
    q->size++;
}

/*
 * Dequeue the PCB at the front of the queue
 */
PcbPtr dequeue(Queue *q) {
    if (!q || q->head == NULL) return NULL;

    PcbPtr p = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    p->next = NULL;
    q->size--;
    return p;
}

/*
 * Peek at the PCB at the head of the queue without removing it
 */
PcbPtr peek(const Queue *q) {
    if (!q) return NULL;
    return q->head;
}

/*
 * Print all PCBs currently in the queue
 */
void printQueue(const Queue *q) {
    if (!q || q->head == NULL) {
        printf("Queue is empty.\n");
        return;
    }
    printf("Queue contents (size = %d):\n", q->size);
    PcbPtr curr = q->head;
    int idx = 1;
    while (curr) {
        printf("  [%d] ", idx++);
        printPcb(curr);
        curr = curr->next;
    }
}

/*
 * Clean up all remaining PCBs in queue
 */
void freeQueue(Queue *q) {
    if (!q) return;
    PcbPtr curr = q->head;
    while (curr) {
        PcbPtr next = curr->next;
        freePcb(curr);
        curr = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

/*
 * Classic single-pointer linked list enqPcb helper
 */
PcbPtr enqPcb(PcbPtr head, PcbPtr p) {
    if (!p) return head;
    p->next = NULL;
    if (!head) return p;

    PcbPtr curr = head;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = p;
    return head;
}

/*
 * Classic single-pointer linked list deqPcb helper
 */
PcbPtr deqPcb(PcbPtr *head) {
    if (!head || !*head) return NULL;
    PcbPtr p = *head;
    *head = (*head)->next;
    p->next = NULL;
    return p;
}
