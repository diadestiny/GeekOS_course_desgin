/*
 * Kernel
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <kernel.h>

DEF_SYSCALL(Print_System_Info,SYS_PRINTSYSINFO,int,(int s),int arg0 = s;,SYSCALL_REGS_1)
DEF_SYSCALL(Select_Paging_Algorithm,SYS_SELECTPAGINGALGORITHM,int,(int s),int arg0 = s;,SYSCALL_REGS_1)
DEF_SYSCALL(SBrk,SYS_SBRK,void*,(ulong_t s),int arg0 = s;,SYSCALL_REGS_1)

