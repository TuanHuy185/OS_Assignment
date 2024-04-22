
#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;


#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++){
		mlq_ready_queue[i].size = 0;
		mlq_ready_queue[i].timeslot = MAX_PRIO - i;
	}
		
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
	
struct pcb_t * get_mlq_proc(void) {
	static int curr_prio = 0;
	struct pcb_t * proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */


// Check if the current queue is not empty and has remaining time slots
	usleep(10);
    if (mlq_ready_queue[curr_prio].timeslot > 0) {
		
		if(!empty(&mlq_ready_queue[curr_prio])){
			pthread_mutex_lock(&queue_lock);
        	proc = dequeue(&mlq_ready_queue[curr_prio]);
			mlq_ready_queue[curr_prio].timeslot--;
			pthread_mutex_unlock(&queue_lock);
		}
		else{
			// Find the next non-empty queue with processes but remain time slot
			pthread_mutex_lock(&queue_lock);
			int temp = (curr_prio + 1) % MAX_PRIO;
			while (temp != curr_prio) {
				if (!empty(&mlq_ready_queue[temp])) {
					proc = dequeue(&mlq_ready_queue[temp]);
					mlq_ready_queue[temp].timeslot--;
					break;
				} else {
					temp = (temp + 1) % MAX_PRIO;
				}
			}
			pthread_mutex_unlock(&queue_lock);
			//Recheck the current queue for ensure
			if (!proc && !empty(&mlq_ready_queue[curr_prio])) {
				pthread_mutex_lock(&queue_lock);
				proc = dequeue(&mlq_ready_queue[curr_prio]);
				mlq_ready_queue[curr_prio].timeslot--;
				pthread_mutex_unlock(&queue_lock);
			}
		}
    } else {
        // Reset timeslot of the current queue
        mlq_ready_queue[curr_prio].timeslot = MAX_PRIO - curr_prio;

        // Find the next non-empty queue with processes
        int temp = (curr_prio + 1) % MAX_PRIO;
        while (temp != curr_prio) {
            if (!empty(&mlq_ready_queue[temp])) {
                curr_prio = temp; // Switch to the queue with processes
				pthread_mutex_lock(&queue_lock);
                proc = dequeue(&mlq_ready_queue[curr_prio]);
				mlq_ready_queue[curr_prio].timeslot--;
				pthread_mutex_unlock(&queue_lock);
                break;
            } else {
                temp = (temp + 1) % MAX_PRIO;
            }
        }

        // If all queues except the current one don't have processes, recheck the current queue
        if (!proc && !empty(&mlq_ready_queue[curr_prio])) {
			pthread_mutex_lock(&queue_lock);
            proc = dequeue(&mlq_ready_queue[curr_prio]);
            mlq_ready_queue[curr_prio].timeslot--;
			pthread_mutex_unlock(&queue_lock);
        }
    }




	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {

	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	usleep(1);
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	return proc;
}

void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif


