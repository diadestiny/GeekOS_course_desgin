/*
 * Message queues
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2004, Iulian Neamtiu <neamtiu@cs.umd.edu>
 * $Revision$
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <geekos/syscall.h>
#include <string.h>
#include <mq.h>

DEF_SYSCALL(Message_Queue_Create,SYS_MESSAGEQUEUE_CREATE,int,(const char *name, ulong_t queue_size),
    const char *arg0 = name; size_t arg1 = strlen(name); ulong_t arg2 = queue_size;,
    SYSCALL_REGS_3)
DEF_SYSCALL(Message_Queue_Destroy,SYS_MESSAGEQUEUE_DESTROY,int,(int mqid),int arg0 = mqid;,SYSCALL_REGS_1)
DEF_SYSCALL(Message_Queue_Send,SYS_MESSAGEQUEUE_SEND,int, (int mqid, void *buffer, ulong_t message_size),
    int arg0 = mqid; void *arg1 = buffer; ulong_t arg2 = message_size;,
    SYSCALL_REGS_3)
DEF_SYSCALL(Message_Queue_Receive,SYS_MESSAGEQUEUE_RECEIVE,int, (int mqid, void *buffer, ulong_t message_size),
    int arg0 = mqid; void *arg1 = buffer; ulong_t arg2 = message_size;,
    SYSCALL_REGS_3)


