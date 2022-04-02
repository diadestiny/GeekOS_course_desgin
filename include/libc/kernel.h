/*
 * Kernel
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <geekos/paging.h>

#define SYS_INFO_PAGING		1
#define SYS_INFO_SCHEDULER	2

int Print_System_Info (int flags);
int Select_Paging_Algorithm (int alg);
void *SBrk(ulong_t increment);


#endif  /* KERNEL_H */

