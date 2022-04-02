/*
 * Kernel threads
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.49 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */
 
 /*  
  * SCHEDULER CHARACTERISTICS
  
  	RoundRobin:
		- One Run(Wait)-Queues
		- Thread with highest priority is always scheduled first
		
	Multilevel Feedback Queue
		- 4 Run(Wait)-Queues (0-3)
		- Threads from queues with highest priority are always scheduled first
				(queue 0 has highest priority)
		- FirstComeFirstServed Policy in all queues
		- New threads are always put on end of queue with highest priority
		- if thread does not complete within quantum,
				then the thread is put on the end of the next queue (lower priority)
		- thread will be promoted to the next queue with higher priority, if thread was blocked before
		
		
  */

#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/symbol.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/scheduler.h>
#include <geekos/errno.h>


#ifdef DEBUG
#define SCHEDULE_DEBUG 1
#endif

// currently used scheduling algorithm
volatile int g_currentScheduleAlgorithm = SCHEDULE_ROUNDROBIN;

// number of context switches so far
volatile ulong_t g_contextSwitches = 0;

/*
 * Run queues.  0 is the highest priority queue.
 */
static struct Thread_Queue s_runQueue[MAX_QUEUE_LEVEL];


#ifdef SCHEDULE_DEBUG
/*
 * Print all running queues and all threads
 */
void PrintRunQueues (char *s) {
    struct Kernel_Thread *kthread;
    int laufQ;
    Print("Printing Queues: %s\n", s);
    for (laufQ=0; laufQ < MAX_QUEUE_LEVEL; laufQ++) {
        kthread = Get_Front_Of_Thread_Queue(&s_runQueue[laufQ]);
        Print("  Queue: %d\n",laufQ);

        while (kthread)
        {
            Print("    pid=%d, prio=%d, readyQueue=%d, blocked=%d, alive=%d\n",
                kthread->pid, kthread->priority, kthread->currentReadyQueue,
                kthread->blocked, kthread->alive);
            kthread = Get_Next_In_Thread_Queue(kthread);
        }
    }

    Print("  All threads: %s\n", s);
    kthread = Get_FirstOfAllThreads();
    while (kthread) {
        Print("    pid=%d, prio=%d, readyQueue=%d, blocked=%d, alive=%d\n",
            kthread->pid, kthread->priority, kthread->currentReadyQueue,
            kthread->blocked, kthread->alive);
        kthread = Get_NextOfAllThreads(kthread);
    }
}
#endif



/*
 * switch to specified scheduling algorithm
 */
int Switch2SchedulingPolicy(int policy, int quantum)
{
    int rc;

#ifdef SCHEDULE_DEBUG
    Print ("About to set policy to %d using %d quantum\n", policy, quantum);
#endif

    if (quantum <= 0)  return EINVALID;

    switch (policy) {
        case SCHEDULE_ROUNDROBIN:    // switch to round-robin
#ifdef SCHEDULE_DEBUG
            PrintRunQueues("Switch2SchedulingPolicy(): SCHEDULE_ROUNDROBIN 1");
#endif
            // put all processes in queue with prio 0
            rc = Move_All_Threads_To_Wait_Queue(0);
            KASSERT(rc==1);                
#ifdef SCHEDULE_DEBUG
            PrintRunQueues("Switch2SchedulingPolicy(): SCHEDULE_ROUNDROBIN 2");
#endif
            break;

        case SCHEDULE_MLF:    	// switch to MLF
#ifdef SCHEDULE_DEBUG
            PrintRunQueues("Switch2SchedulingPolicy(): SCHEDULE_MLF 1");
#endif
            // find the system idle process and put into queue with lowest priority
            rc = Move_Idle_To_Queue(MAX_QUEUE_LEVEL-1);
            KASSERT(rc==1);
#ifdef SCHEDULE_DEBUG
            PrintRunQueues("Switch2SchedulingPolicy(): SCHEDULE_MLF 2");
#endif
            break;

        default:
            return EINVALID;
    }

    g_currentScheduleAlgorithm = policy;
    g_Quantum = quantum;
    return 0;
}


/*
 * Add given thread to the run queue, so that it
 * may be scheduled.  Must be called with interrupts disabled!
 */
void Make_Runnable(struct Kernel_Thread* kthread)
{
	int currentQ;
    KASSERT(!Interrupts_Enabled());
			
	// for MLF schedulung: promote thread higher priority if thread was blocked before
	if ((g_currentScheduleAlgorithm ==SCHEDULE_MLF) && (kthread->blocked == true) && (kthread->currentReadyQueue > 0)) 
	{
		kthread->currentReadyQueue--;
		#ifdef SCHEDULE_DEBUG
				Print("process %d promoted to ready queue %d because it was blocked before\n", kthread->pid, kthread->currentReadyQueue);
		#endif		
	}
		
    currentQ = kthread->currentReadyQueue;
    KASSERT(currentQ >= 0 && currentQ < MAX_QUEUE_LEVEL);
    kthread->blocked = false;
    Enqueue_Thread(&s_runQueue[currentQ], kthread);
}


/*
 * Atomically make a thread runnable.
 * Assumes interrupts are currently enabled.
 */
void Make_Runnable_Atomic(struct Kernel_Thread* kthread)
{
    bool iflag = Begin_Int_Atomic();
    Make_Runnable(kthread);
    End_Int_Atomic(iflag);
}


/*
 * move all threads to one queue
 */
int Move_All_Threads_To_Wait_Queue(int toQueue)
{
#ifdef SCHEDULE_DEBUG
    Print("Move_All_Threads_To_Wait_Queue(): to=%d\n", toQueue);
#endif
    struct Kernel_Thread *kthread;
    int currentWaitQueue;
    
    if (toQueue >= MAX_QUEUE_LEVEL || toQueue < 0)
        return 0;
    
    kthread = Get_FirstOfAllThreads();

    while (kthread) 
    {
#ifdef SCHEDULE_DEBUG
        Print ("Found thread %d, prio=%d, readyQueue=%d, blocked=%d, alive=%d\n",
                kthread->pid, kthread->priority, kthread->currentReadyQueue,
                kthread->blocked, kthread->alive);
#endif
        currentWaitQueue = kthread->currentReadyQueue;
        if (currentWaitQueue != toQueue) {
            // if thread is in wait-queue then move to another queue
            if (Is_Member_Of_Thread_Queue(&s_runQueue[currentWaitQueue],kthread))
            {
#ifdef SCHEDULE_DEBUG
                Print ("Moving thread from wait-queue %d to wait-queue %d\n", currentWaitQueue, toQueue);
#endif
                bool iflag = Begin_Int_Atomic();
                Remove_Thread(&s_runQueue[currentWaitQueue], kthread);
                Enqueue_Thread(&s_runQueue[toQueue], kthread);
                End_Int_Atomic(iflag);
            }
            kthread->currentReadyQueue = toQueue;
        }
        kthread = Get_NextOfAllThreads(kthread);
    }

    return 1;
}


/*
 * move idle threads to specified queue
 */
int Move_Idle_To_Queue(int toQueue)
{
    struct Kernel_Thread *kthread;
    int laufQ;
#ifdef SCHEDULE_DEBUG
    Print("move idle thread to queue %d\n", toQueue);
#endif

    if (toQueue >= MAX_QUEUE_LEVEL || toQueue < 0)  return 0;

    // idle-thread must not be blocked, so it should be sufficient to look for
    // the idle-thread in the run(wait) queues

    for (laufQ=0; laufQ < MAX_QUEUE_LEVEL-1; laufQ++)
    {
        kthread = Get_Front_Of_Thread_Queue(&s_runQueue[laufQ]);    

        while (kthread) 
        {
            if (kthread->priority == PRIORITY_IDLE) 
            {
#ifdef SCHEDULE_DEBUG
                Print("found idle thread %d in queue %d\n",kthread->pid, laufQ);
#endif
                if (laufQ == toQueue)  return 1;

                bool iflag = Begin_Int_Atomic();
                Remove_Thread(&s_runQueue[laufQ], kthread);
                Enqueue_Thread(&s_runQueue[toQueue], kthread);
                End_Int_Atomic(iflag);
                kthread->currentReadyQueue = toQueue;
                return 1;
            }

            kthread = Get_Next_In_Thread_Queue(kthread);
        }
    }

    return 1;
}

/* 
 * determine next run/wait-queue for this thread
 */

int Set_ThreadWaitQueueByReschedule(struct Kernel_Thread* kthread)
{
	KASSERT(kthread!=0);
	switch (g_currentScheduleAlgorithm) 
	{
       	case SCHEDULE_MLF: // denote thread
			if (kthread->currentReadyQueue < (MAX_QUEUE_LEVEL - 1)) 
			{
				kthread->currentReadyQueue++;
#ifdef SCHEDULE_DEBUG
				Print("process %d moved to ready queue %d\n", kthread->pid, kthread->currentReadyQueue);
#endif				
			}
  			break;
   
    	default:		
           	break;
    }
	return 1;
}


/*
 * Get the next runnable thread from the run queue.
 * This is the scheduler.
 */
struct Kernel_Thread* Get_Next_Runnable(void)
{
    struct Kernel_Thread* best = 0;
    struct Thread_Queue* q = s_runQueue;

    /* Find the best thread from the highest-priority run queue */
    //TODO("Find a runnable thread from run queues");

    switch (g_currentScheduleAlgorithm) {
        case SCHEDULE_MLF:	// find next thread for MLF
            while (q < s_runQueue + MAX_QUEUE_LEVEL) {
                if ( (best = Find_Next(q)) )     		
					break;
                q++;
            }
#ifdef SCHEDULE_DEBUG
            if (best == 0)  PrintRunQueues("Get_Next_Runnable(): SCHEDULE_MLF best == 0");
#if 0
            /* do not activate this, it lasts too long */
            PrintRunQueues("Get_Next_Runnable(): SCHEDULE_MLF");
            if (best)  Print("Best found is %d, laufQ=%d\n", best->pid, q - s_runQueue);
#endif
#endif
            KASSERT(best != 0);
            break;

        default:		// find next thread for round robin
            // we only need to look at queue 0
            best = Find_Best(q);
#ifdef SCHEDULE_DEBUG
            if (best == 0)  PrintRunQueues("Get_Next_Runnable(): default best == 0");
#endif
            KASSERT(best != 0);
            break;
    }

    bool iflag = Begin_Int_Atomic();
    Remove_Thread(q, best);
    End_Int_Atomic(iflag);

    //Print("Scheduling %x\n", best->pid);
    ++g_contextSwitches;

    return best;
}


/*
 * Dump some information of the scheduler to the console
 */
void Dump_Scheduler_Info(void) {
    char *alg = "unknown";

    switch (g_currentScheduleAlgorithm) {
        case SCHEDULE_MLF: alg = "MLF"; break;
        default: alg = "RR"; break;
    }

    Print ("Scheduler algorithm %s, quantum %d, context switches %ld\n",
           alg, g_Quantum, g_contextSwitches);
}

