/*
 * Message Queue operations
 *
 */

#ifndef MQ_H
#define MQ_H

#include <geekos/ktypes.h>
#include <geekos/mqueue.h>

int Message_Queue_Create(const char *name, ulong_t queue_size);
int Message_Queue_Destroy(int mqid);
int Message_Queue_Send(int mqid, void * buffer, ulong_t message_size);
int Message_Queue_Receive(int mqid, void * buffer, ulong_t message_size);

#endif  /* MQ_H */

