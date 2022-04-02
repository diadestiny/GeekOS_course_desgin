/*
 * System Information
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/sysinfo.h>
#include <geekos/string.h>
#include <geekos/paging.h>
#include <geekos/scheduler.h>
#include <libc/kernel.h>


#ifdef DEBUG
#ifndef SYSINFO_DEBUG
#define SYSINFO_DEBUG
#endif
#endif


/*
 * Print System Information to screen
 */
int PrintSystemInfo(int flags) {

    extern volatile ulong_t g_numTicks;

    Print ("System Information:\nTime ticks=%ld.\n", g_numTicks);

    if (flags & SYS_INFO_PAGING)     Dump_Paging_Info();
    if (flags & SYS_INFO_SCHEDULER)  Dump_Scheduler_Info();

    return 0;
}
