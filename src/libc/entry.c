/*
 * User program entry point function
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.10 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <geekos/argblock.h>

int main(int argc, char **argv);
void Exit(int exitCode);
void Init_Heap(ulong_t start, ulong_t size);

/*
 * Entry point.  Calls user program's main() routine, then exits.
 */
void _Entry(void)
{
    struct Argument_Block *argBlock;
    ulong_t maxAddr;
    ulong_t sz;

    /* The argument block pointer is in the ESI register. */
    __asm__ __volatile__ ("movl %%esi, %0" : "=r" (argBlock));

    /* The size of the program is in the EAX register and the initial heap size in the EBX register */
    __asm__ __volatile__ ("movl %%eax, %0" : "=r" (maxAddr));
    __asm__ __volatile__ ("movl %%ecx, %0" : "=r" (sz));
    //Print ("_Entry: user heap start=0x%08lx size=0x%08lx\n", maxAddr, sz);

    Init_Heap(maxAddr, sz);

    /* Call main(), and then exit with whatever value it returns. */
    Exit(main(argBlock->argc, argBlock->argv));
}

