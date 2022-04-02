/*
 * Message Queues
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/ktypes.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/kassert.h>
#include <geekos/mqueue.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/errno.h>


#ifdef DEBUG
#ifndef MQ_DEBUG
#define MQ_DEBUG
#endif
#endif

#ifdef MQ_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif


static struct Msg_Queue_List s_messageQueueList;

static ulong_t s_nextMQid = 0;   /* next message queue id */


/*
 * 转储当前消息队列
 */
void MQ_Dump(void) {
    struct Msg_Queue *mq;

    mq = Get_Front_Of_Msg_Queue_List (&s_messageQueueList);
    while (mq != 0) {
        Print ("Message Queue: %s  flags=%d, maxmsg=%d, msgsize=%d, curmsgs=%d, users=%d\n",
            mq->name, mq->flags, mq->maxmsg, mq->msgsize, mq->curmsgs, mq->users);
        mq = Get_Next_In_Msg_Queue_List (mq);
    }
}


/*
 * 消息队列的初始化 
 */
void Init_MQ(void) {
   Clear_Msg_Queue_List (&s_messageQueueList);
}


/*
 * 创建消息队列MQ
 *
 * Returns: the global MQ id
 * 如果MQ不存在，它将创建MQ，如果MQ存在，则返回已经存在的ID。 
 */

int MQ_Create(char *name, ulong_t queue_size) {

    Debug("MQ_Create: name='%s', size=%ld\n", name, queue_size);
    struct Msg_Queue *mq;

    KASSERT(!Interrupts_Enabled());

    mq = Get_Front_Of_Msg_Queue_List (&s_messageQueueList);
    while (mq != 0) {
        if (strcmp(mq->name, name) == 0)
            break;
        mq = Get_Next_In_Msg_Queue_List (mq);
    }

    if (mq) {
        mq->users++;
        return mq->id;
    }

    mq = (struct Msg_Queue*) Malloc (sizeof(*mq));
    if (mq) {
        mq->name = name;
        mq->id = ++s_nextMQid;
        mq->flags = 0;
        mq->maxmsg = queue_size;
        mq->msgsize = MESSAGE_MAX_SIZE;
        mq->curmsgs = 0;
        mq->users = 1;
        Clear_Thread_Queue (&mq->rdQueue);
        Clear_Thread_Queue (&mq->wrQueue);
        Clear_Msg_List (&mq->msgList);
        Add_To_Back_Of_Msg_Queue_List (&s_messageQueueList, mq);
        return mq->id;
    }

    if (name) Free (name);
    return ENOMEM;
}


/*
 * 销毁消息队列
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 * 仅销毁该线程的MQ。 您需要一个引用计数。
*  如果ref计数变为0，则完全销毁MQ。 
 */

int MQ_Destroy(int mqid) {

    Debug("MQ_Destroy: mqid=%d\n", mqid);
    struct Msg_Queue *mq;
    int rc = ENOTFOUND;

    KASSERT(!Interrupts_Enabled());

    if ( Is_Msg_Queue_List_Empty (&s_messageQueueList) )
        return ENOTFOUND;

    mq = Get_Front_Of_Msg_Queue_List (&s_messageQueueList);

    while (mq != 0) {
        if (mq->id == mqid)
            break;
        mq = Get_Next_In_Msg_Queue_List (mq);
    }

    if (mq) {
        mq->users--;
        if ( mq->users ) {
            Debug("MQ_Destroy: mqid=%d rc=0 (users left)\n", mqid);
            rc = 0;
        } else {
            if ( !Is_Msg_List_Empty (&mq->msgList) ) {
                Debug("MQ_Destroy: mqid=%d rc=EBUSY (messages left)\n", mqid);
                rc = EBUSY;
            } else {
                Remove_From_Msg_Queue_List (&s_messageQueueList, mq);
                Free (mq->name);
                Free (mq);
                Debug("MQ_Destroy: mqid=%d, deleted\n", mqid);
                rc = 0;
            }
        }
    }

    return rc;
}


/*
 * 写入消息队列
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 * 缓冲区已满
 */

int MQ_Send(int mqid, struct Msg* msg) {

    Debug("MQ_Send: mqid=%d, buffer=%p, size=%ld\n", mqid, msg->data, msg->size);

    struct Msg_Queue *mq;

    KASSERT(!Interrupts_Enabled());

    if ( Is_Msg_Queue_List_Empty (&s_messageQueueList) )
        return ENOTFOUND;

    mq = Get_Front_Of_Msg_Queue_List (&s_messageQueueList);

    while (mq && mq->id != mqid)
        mq = Get_Next_In_Msg_Queue_List (mq);

    if (!mq)
        return ENOTFOUND;

    Debug ("MQ_Send: name=%s, flags=%d, maxmsg=%d, msgsize=%d, curmsgs=%d, users=%d\n",
        mq->name, mq->flags, mq->maxmsg, mq->msgsize, mq->curmsgs, mq->users);

    while (mq->curmsgs >= mq->maxmsg)
        Wait(&mq->wrQueue);

    mq->curmsgs++;
    Add_To_Back_Of_Msg_List (&mq->msgList, msg);
    Wake_Up_One (&mq->rdQueue);

    return 0;
}


/*
 * 从消息队列中读取 
 * Returns: a Message if successful, 0 if unsuccessful
 * 如果没有可用数据，则阻止
 */

struct Msg* MQ_Receive(int mqid) {

    Debug ("MQ_Receive: mqid=%d\n", mqid);

    KASSERT(!Interrupts_Enabled());

    struct Msg_Queue *mq;
    struct Msg *msg;

    if ( Is_Msg_Queue_List_Empty (&s_messageQueueList) )
        return 0;

    Debug ("MQ_Receive: 1\n");
    mq = Get_Front_Of_Msg_Queue_List (&s_messageQueueList);

    while (mq && mq->id != mqid) {
        Debug ("MQ_Receive: 2 %p\n", mq);
        mq = Get_Next_In_Msg_Queue_List (mq);
    }

    if (!mq)
        return 0;

    Debug ("MQ_Receive: name=%s, flags=%d, maxmsg=%d, msgsize=%d, curmsgs=%d, users=%d\n",
        mq->name, mq->flags, mq->maxmsg, mq->msgsize, mq->curmsgs, mq->users);

    while (mq->curmsgs == 0) {
        Debug ("MQ_Receive: waiting for queue %s\n", mq->name);
        Wait (&mq->rdQueue);
    }

    mq->curmsgs--;
    msg = Remove_From_Front_Of_Msg_List (&mq->msgList);

    Wake_Up_One (&mq->wrQueue);

    return msg;
}


