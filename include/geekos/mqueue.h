/*
 * Message queues system calls header
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2004, Iulian Neamtiu <neamtiu@cs.umd.edu>
 * $Revision$
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef GEEKOS_MQUEUE_H
#define GEEKOS_MQUEUE_H


#define MQUEUE_MAX_SIZE 4096

/* in theory, there's no reason to limit MESSAGE_MAX_SIZE,
   but we want to avoid pathologically large messages */
#define MESSAGE_MAX_SIZE (2 * MQUEUE_MAX_SIZE)

#ifndef MQ_H

#include <geekos/kthread.h>

struct Msg;

DEFINE_LIST(Msg_List, Msg);

struct Msg {
    void* data;      /* The message data  */
    ulong_t size;    /* The message size  */
    DEFINE_LINK(Msg_List, Msg);
};

IMPLEMENT_LIST(Msg_List, Msg);


struct Msg_Queue;

DEFINE_LIST(Msg_Queue_List, Msg_Queue);

struct Msg_Queue {
    char* name;      /* Message queue name */
    int    id;       /* Message queue id */
    uint_t flags;    /* Message queue flags.  */
    uint_t maxmsg;   /* Maximum number of messages.  */
    uint_t msgsize;  /* Maximum message size.  */
    uint_t curmsgs;  /* Number of messages currently queued.  */
    uint_t users;    /* Number of message queue users  */
    struct Thread_Queue rdQueue;  /* Queue for readers  */
    struct Thread_Queue wrQueue;  /* Queue for writers  */
    struct Msg_List msgList;
    DEFINE_LINK(Msg_Queue_List, Msg_Queue);
};

IMPLEMENT_LIST(Msg_Queue_List, Msg_Queue);


void Init_MQ(void);
int MQ_Create(char *name, ulong_t queue_size);
int MQ_Destroy(int mqid);
int MQ_Send(int mqid, struct Msg* msg);
struct Msg* MQ_Receive(int mqid);

#endif  /* MQ_H */

#endif  /* GEEKOS_MQUEUE_H */
