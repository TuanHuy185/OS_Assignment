#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (!q || !proc)
                return;
        if (q->size >= MAX_QUEUE_SIZE)
                return;
        q->proc[q->size] = proc;
        q->size++;
}
struct pcb_t *get_proc(void)
{
     pthread_mu
}
struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q->size <= 0)
        {
                return NULL;
        }
        int highestPriorityIndex = 0;
        for (int i = 1; i < q->size; i++)
        {
                if (q->proc[i]->prio < q->proc[highestPriorityIndex]->prio)
                        highestPriorityIndex = i;
        }
        struct pcb_t *highest_priority_proc = q->proc[highestPriorityIndex];
        for (int i = highestPriorityIndex; i < q->size - 1; i++)
        {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;
        return highest_priority_proc;
}


