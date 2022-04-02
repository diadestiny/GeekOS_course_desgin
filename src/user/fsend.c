/* A fragmented send-receive test for Project 6
 * This is the sender process
 *
 * Iulian Neamtiu
 *
 */

#include <conio.h>
#include <mq.h>
#include <process.h>
#include <string.h>

#define MAILBOX_SIZE 10
#define BUFFER_SIZE 21
#define NR_MESSAGES 10

char * process_name = "Sender";

int main(int argc , char ** argv)
{
  int sendMQ;	/* id of MQ */
  char buffer[BUFFER_SIZE];
  int pid, i;

  pid = Get_PID();
  Print ("Starting process %d (%s) buffer size is %d ...\n", pid, process_name, BUFFER_SIZE) ;

  sendMQ = Message_Queue_Create("fragmentation", MAILBOX_SIZE);

  for (i=0; i < NR_MESSAGES; i++) 
    {
      snprintf (buffer, BUFFER_SIZE, "%d. Message", i+1);
      Message_Queue_Send(sendMQ, buffer, BUFFER_SIZE);
      Print("Process %d (%s) sent %d bytes: '%s'\n", pid, process_name, BUFFER_SIZE, buffer);
    }

  Message_Queue_Destroy(sendMQ);

  Print ("\nProcess %d (%s) is done !\n", pid, process_name) ;

  return 0;
}
