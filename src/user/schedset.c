/*
 * Scheduleset: sets scheduling algorithm and quantum
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>

#if !defined (NULL)
#define NULL 0
#endif

int main(int argc , char ** argv)
{
  int policy = -1;
  int quantum;

  if (argc == 3) {
      if (!strcmp(argv[1], "rr")) {
          policy = 0;
      } else if (!strcmp(argv[1], "mlf")) {
          policy = 1;
      } else {
	  Print("usage: %s [rr|mlf] <quantum>\n", argv[0]);
	  Exit(1);
      }
      quantum = atoi(argv[2]);
      Set_Scheduling_Policy(policy, quantum);
  } else {
      Print("usage: %s [rr|mlf] <quantum>\n", argv[0]);
      Exit(1);
  }
  
  return 0;
}
