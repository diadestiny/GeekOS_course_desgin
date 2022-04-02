/*
 * Kernel threads
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.49 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <stddef.h>
#include <kernel.h>

#include <geekos/defs.h>
#include <geekos/bget.h>

#ifdef DEBUG
#ifndef LIBC_MALLOC_DEBUG
#define LIBC_MALLOC_DEBUG
#endif
#endif

#ifdef LIBC_MALLOC_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif


void *Heap_Extender(bufsize size) {
    Debug ("Heap_Extender %ld\n", size);
    return SBrk (size);
}

/*
 * Initialize the heap starting at given address and occupying
 * specified number of bytes.
 */
void Init_Heap(ulong_t start, ulong_t size)
{
    Debug("Creating user heap: start=0x%lx, size=%ld\n", start, size);
    bpool((void*) start, size);
    bectl (0, Heap_Extender, 0, PAGE_SIZE);
}

/*
 * Dynamically allocate a buffer of given size.
 * Returns null if there is not enough memory to satisfy the
 * allocation.
 */
void *Malloc(size_t size)
{
//  bool iflag;

    void *result;

//  iflag = Begin_Int_Atomic();
    result = bget(size);
//  End_Int_Atomic(iflag);

    Debug ("Malloc: p=%p\n", result);

    return result;
}

/*
 * Free a buffer allocated with Malloc() or Malloc().
 */
void Free(void* buf)
{
//  bool iflag;

    Debug ("Free: p=%p\n", buf);

//  iflag = Begin_Int_Atomic();
    brel(buf);
//  End_Int_Atomic(iflag);
}

