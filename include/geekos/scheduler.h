/*
 * Scheduler
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */
 
#ifndef GEEKOS_SCHEDULER_H
#define GEEKOS_SCHEDULER_H

#include <geekos/kthread.h>

#define SCHEDULE_ROUNDROBIN	0
#define SCHEDULE_MLF	1

/*
 * Number of ready queue levels.
 */
#define MAX_QUEUE_LEVEL 4



// currently used scheduling algorithm
extern volatile int g_currentScheduleAlgorithm;

// number of context switches so far
volatile ulong_t g_contextSwitches;

/*
 * Find the best (highest priority) thread in given
 * thread queue.  Returns null if queue is empty.
 */
static __inline__ struct Kernel_Thread* Find_Best(struct Thread_Queue* queue)
{
    /* Pick the highest priority thread */
    struct Kernel_Thread *kthread = queue->head, *best = 0;
    while (kthread != 0) {
	if (best == 0 || kthread->priority > best->priority)
	    best = kthread;
	kthread = Get_Next_In_Thread_Queue(kthread);
    }
    return best;
}

/*
 * Get next thread from given
 * thread queue.  Returns null if queue is empty.
 */
static __inline__ struct Kernel_Thread* Find_Next(struct Thread_Queue* queue)
{
    return queue->head;
}
	
void Make_Runnable(struct Kernel_Thread* kthread);
void Make_Runnable_Atomic(struct Kernel_Thread* kthread);
	

// Get the next runnable thread from the run queue.
struct Kernel_Thread* Get_Next_Runnable(void);


// move all threads from one queue to another
int Move_Threads_Between_Queue(int queue1, int queue2);


// move ide threds to specified queue
int Move_Idle_To_Queue(int toQueue);

// move all threads to this queue
int Move_All_Threads_To_Wait_Queue(int toQueue);

// switch to specified scheduling algorithm
int Switch2SchedulingPolicy(int policy, int quantum);

// determine next run/wait-queue for this thread
int Set_ThreadWaitQueueByReschedule(struct Kernel_Thread* kthread);

void Dump_Scheduler_Info(void);

#endif  /* GEEKOS_SCHEDULER_H */
