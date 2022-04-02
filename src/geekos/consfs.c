/*
 * Console pseudo-filesystem
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.11 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/vfs.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/consfs.h>

#ifdef DEBUG
#ifndef CONSFS_DEBUG
#define CONSFS_DEBUG
#endif
#endif

#ifdef CONSFS_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

/*
 * Read from console (keyboard).
 * Returns number of bytes read, or error code.
 */
static int Console_Read(struct File *file, void *buf, ulong_t numBytes)
{
    //TODO("Read from console");
    if (numBytes <= 0 || !file || !buf)
        return EINVALID;

    int i;
    Keycode k;
    for (i=0; i < numBytes; i++) {
       do k = Wait_For_Key(); while ( k & KEY_RELEASE_FLAG );
       *(char*)buf++ = k & 0xFF;
    }
    return i;
}

/*
 * Write to console (screen).
 * Returns number of bytes written.
 */
static int Console_Write(struct File *file, void *buf, ulong_t numBytes)
{
    //TODO("Write to console");
    ulong_t i;
    for (i=0; i < numBytes; i++)
        Put_Char( *(char*)(buf+i) );
    return i;
}

/*
 * Close console input or output file.
 * Returns 0 if successful, error code if not.
 */
static int Console_Close(struct File *file)
{
    //TODO("Close console input or output");
    Debug("Console_Close file=%p\n", file);
    return 0;
}

/*
 * Clone a console File object.
 */
static int Console_Clone(struct File *file, struct File **pClone)
{
    struct File *clone;

    if (file->ops->Write == 0) {
        clone = Open_Console_Input();
        Debug("Console_Clone input  file=%p clone=%p\n", file, clone);
    } else {
        clone = Open_Console_Output();
        Debug("Console_Clone output file=%p clone=%p\n", file, clone);
    }

    if (!clone)
        return ENOMEM;

    *pClone = clone;
    return 0;
}

/*
 * File_Ops for console input.
 */
static struct File_Ops s_consInputFileOps = {
    0,            /* FStat() */
    &Console_Read,
    0,            /* Write() */
    0,            /* Seek() */
    &Console_Close,
    0,            /* Read_Entry() */
    &Console_Clone,
};

/*
 * File_Ops for console output.
 */
static struct File_Ops s_consOutputFileOps = {
    0,            /* FStat() */
    0,            /* Read() */
    &Console_Write,
    0,            /* Seek() */
    &Console_Close,
    0,            /* Read_Entry() */
    &Console_Clone,
};

/*
 * Create a File object for console input or output.
 * Returns null pointer if File can't be created.
 */
static struct File *Do_Open(struct File_Ops *ops, int mode)
{
    return Allocate_File(ops, 0, 0, 0, mode, 0);
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Open a File which will read from the console (keyboard).
 */
struct File *Open_Console_Input(void)
{
    struct File *file = Do_Open(&s_consInputFileOps, O_READ);
    Debug("Open_Console_Input file=%p\n", file);
    return file;
}

/*
 * Open a File which will write to the console (screen).
 */
struct File *Open_Console_Output(void)
{
    struct File *file = Do_Open(&s_consOutputFileOps, O_WRITE);
    Debug("Open_Console_Output file=%p\n", file);
    return file;
}

