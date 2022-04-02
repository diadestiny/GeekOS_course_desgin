/*
 * Pipe pseudo-filesystem
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.12 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kassert.h>
#include <geekos/errno.h>
#include <geekos/vfs.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/pipefs.h>
#include <geekos/kthread.h>
#include <geekos/int.h>

#ifdef DEBUG
#ifndef PIPE_DEBUG
#define PIPE_DEBUG
#endif
#endif

#ifdef PIPE_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif

void Hex_Dump (void* paddr, ulong_t len);

/* The amount of storage to allocate for a pipe. */
#define PIPE_BUF_SIZE 4096

struct Pipe {
    void*       data;           /* the pipe's data buffer  */
    ulong_t     size;           /* the size of the pipe's data buffer  */
    uint_t      references;     /* the number of filedescriptors that references this pipe  */
    ulong_t     rd;             /* read pointer  */
    ulong_t     wr;             /* write pointer  */
    struct Thread_Queue rdQueue;  /* Queue for readers  */
    struct Thread_Queue wrQueue;  /* Queue for writers  */
};


/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

/*
 * Read data from a pipe.
 * Returns number of bytes read, or 0 if end-of-file
 * has been reached.
 */
static int Pipe_Read(struct File *file, void *buf, ulong_t numBytes)
{
    //TODO("Read from pipe");
    Debug("Pipe_Read:  pid=%d file=%p, pipe=%p\n",
          g_currentThread->pid, file, file->fsData);

    if (!file || !file->fsData)
        return EINVALID;

    struct Pipe *p = (struct Pipe*) file->fsData;
    ulong_t avail;

    bool iflag = Begin_Int_Atomic();

    while ( (avail = (p->wr - p->rd) % p->size) == 0 && p->references > 1) {
        Debug("Pipe_Read:  pid=%d wait ref=%d rd=%ld wr=%ld\n",
              g_currentThread->pid, p->references, p->rd, p->wr);
        Wake_Up_One(&p->wrQueue);
        Wait(&p->rdQueue);
    }

    if (p->references == 1 && !avail) {
        End_Int_Atomic(iflag);
        return 0;
    }

    Debug("Pipe_Read:  pid=%d rd=%ld wr=%ld\n",
        g_currentThread->pid, p->rd, p->wr);

    if (avail < numBytes)
        numBytes = avail;

    if (p->rd + numBytes > p->size) {
        ulong_t ofs = p->size - p->rd;
        Debug("Pipe_Read:  pid=%d memcpy 1: buf=%p, src=%p, len=%ld\n",
              g_currentThread->pid, buf, p->data + p->rd, ofs);
        memcpy (buf,     p->data + p->rd, ofs);
        //Hex_Dump (buf, ofs);
        Debug("Pipe_Read:  pid=%d memcpy 2: buf=%p, src=%p, len=%ld\n",
              g_currentThread->pid, buf+ofs, p->data, numBytes-ofs);
        memcpy (buf+ofs, p->data        , numBytes - ofs);
        //Hex_Dump (buf+ofs, numBytes-ofs);
    } else {
        Debug("Pipe_Read:  pid=%d memcpy:   buf=%p, src=%p, len=%ld\n",
              g_currentThread->pid, buf, p->data + p->rd, numBytes);
        memcpy (buf, p->data + p->rd, numBytes);
        //Hex_Dump (buf, numBytes);
    }
    p->rd = (p->rd + numBytes) % p->size;

    Wake_Up_One(&p->wrQueue);

    End_Int_Atomic(iflag);

    return numBytes;
}

/*
 * Write data to pipe.
 * Returns number of bytes written.
 */
static int Pipe_Write(struct File *file, void *buf, ulong_t numBytes)
{
    //TODO("Write to pipe");
    Debug("Pipe_Write: pid=%d file=%p, pipe=%p\n",
          g_currentThread->pid, file, file->fsData);

    if (!file || !file->fsData)
        return EINVALID;

    struct Pipe *p = (struct Pipe*) file->fsData;
    ulong_t avail;

    if (p->references <= 1)
        return 0;

    bool iflag = Begin_Int_Atomic();

    while ( (avail = (p->rd - p->wr) % p->size) == 1 ) {
        Debug("Pipe_Write: pid=%d wait ref=%d rd=%ld wr=%ld\n",
          g_currentThread->pid, p->references, p->rd, p->wr);
        Wake_Up_One(&p->rdQueue);
        Wait(&p->wrQueue);
    }

    --avail;
    Debug("Pipe_Write: pid=%d rd=%ld wr=%ld\n",
         g_currentThread->pid, p->rd, p->wr);

    if (avail < numBytes)
        numBytes = avail;

    if (p->wr + numBytes > p->size) {
        ulong_t ofs = p->size - p->wr;
        Debug("Pipe_Write: pid=%d memcpy 1: buf=%p, src=%p, len=%ld\n",
          g_currentThread->pid, buf, p->data + p->wr, ofs);
        memcpy (p->data + p->wr, buf    , ofs);
        //Hex_Dump (buf, ofs);
        Debug("Pipe_Write: pid=%d memcpy 2: buf=%p, src=%p, len=%ld\n",
          g_currentThread->pid, buf+ofs, p->data, numBytes-ofs);
        memcpy (p->data        , buf+ofs, numBytes - ofs);
        //Hex_Dump (buf+ofs, numBytes-ofs);
    } else {
        Debug("Pipe_Write: pid=%d memcpy:   buf=%p, src=%p, len=%ld\n",
          g_currentThread->pid, buf, p->data + p->wr, numBytes);
        memcpy (p->data + p->wr, buf, numBytes);
        //Hex_Dump (buf, numBytes);
    }
    p->wr = (p->wr + numBytes) % p->size;

    Wake_Up_One(&p->rdQueue);

    End_Int_Atomic(iflag);

    return numBytes;
}

/*
 * Close pipe.
 */
static int Pipe_Close(struct File *file)
{
    //TODO("Close a pipe");
//  Debug("Pipe_Close: pid=%d file=%p, pipe=%p\n",
//        g_currentThread->pid, file, file->fsData);

    if (!file || !file->fsData)
        return EINVALID;

    bool iflag = Begin_Int_Atomic();

    struct Pipe *p = (struct Pipe*) file->fsData;

    --p->references;

    if (p->references > 0) {
        Wake_Up_One(&p->rdQueue);
        Wake_Up_One(&p->wrQueue);
        Debug("Pipe_Close: pid=%d ref=%d leave open. file=%p, pipe=%p\n",
            g_currentThread->pid, p->references, file, file->fsData);
    } else {
        Debug("Pipe_Close: pid=%d ref=%d destroy.    file=%p, pipe=%p\n",
            g_currentThread->pid, p->references, file, file->fsData);
        if (p->data)  Free (p->data);
        Free (p);
    }

    End_Int_Atomic(iflag);

    return 0;
}

/*
 * Clone a pipe.
 */
static int Pipe_Clone(struct File *file, struct File **pClone)
{
    //TODO("Clone a pipe");
    int rc = 0;
    struct File *clone;
    struct Pipe *p = 0;

    /* Create a duplicate File object. */
    clone = Allocate_File (file->ops, file->filePos, file->endPos, file->fsData,
                           file->mode, file->mountPoint);
    if (clone == 0) {
        rc = ENOMEM;
        goto done;
    }

    *pClone = clone;

    p = (struct Pipe*) file->fsData;
    bool iflag = Begin_Int_Atomic();
    ++p->references;
    End_Int_Atomic(iflag);

    //Hex_Dump (p, PIPE_BUF_SIZE);

done:
    Debug ("Pipe_Clone: pid=%d ref=%d, file=%p, clone=%p\n",
           g_currentThread->pid, p->references, file, clone);
    return rc;
}


static struct File_Ops s_readPipeFileOps = {
    0,			/* FStat() */
    &Pipe_Read,
    0,			/* Write() */
    0,			/* Seek() */
    &Pipe_Close,
    0,			/* Read_Entry() */
    &Pipe_Clone,
};

static struct File_Ops s_writePipeFileOps = {
    0,			/* FStat() */
    0,			/* Write() */
    &Pipe_Write,
    0,			/* Seek() */
    &Pipe_Close,
    0,			/* Read_Entry() */
    &Pipe_Clone,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

int Create_Pipe(struct File **pRead, struct File **pWrite)
{
    int rc = 0;
    struct File *read = 0, *write = 0;
    struct Pipe *p = 0;

    /*
     * TODO: you should allocate a data structure for managing the
     * pipe, and store it in the fsData field of the read and
     * write pipes.  It will need a reference count field
     * in order to destroy it when the last File connected
     * to the pipe is closed.
     */
    p = (struct Pipe*) Malloc (sizeof(*p));
    if (!p) {
        rc = ENOMEM;
        goto done;
    }

    p->data = (void*) Malloc (PIPE_BUF_SIZE);
    if (!p->data) {
        Free (p);
        rc = ENOMEM;
        goto done;
    }

    p->size = PIPE_BUF_SIZE;
    p->references = 2;
    p->rd = 0;
    p->wr = 0;
    Clear_Thread_Queue(&p->rdQueue);
    Clear_Thread_Queue(&p->wrQueue);

    /* Allocate File objects */
    if ((read  = Allocate_File(&s_readPipeFileOps,  0, 0, p, O_READ,  0)) == 0 ||
	(write = Allocate_File(&s_writePipeFileOps, 0, 0, p, O_WRITE, 0)) == 0   ) {
	rc = ENOMEM;
    }

    *pRead = read;
    *pWrite = write;


done:
    if (rc < 0) {
	if (!read)  Free(read);
	if (!write) Free(write);
        if (p != 0) {
            if (p->data != 0)  Free (p->data);
            Free (p);
        }
    }
    return rc;
}

