/*
 * Common user mode functions
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.50 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/ktypes.h>
#include <geekos/kassert.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/kthread.h>
#include <geekos/vfs.h>
#include <geekos/tss.h>
#include <geekos/user.h>

#ifdef DEBUG
#ifndef USER_DEBUG
#define USER_DEBUG
#endif
#endif

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

#ifdef USER_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif


/*
 * This module contains common functions for implementation of user
 * mode processes.
 */

/*
 * Associate the given user context with a kernel thread.
 * This makes the thread a user process.
 */
void Attach_User_Context(struct Kernel_Thread* kthread, struct User_Context* context)
{
    Debug ("Attach_User_Context()\n");

    KASSERT(context != 0);
    kthread->userContext = context;

    Disable_Interrupts();

    /*
     * We don't actually allow multiple threads
     * to share a user context (yet)
     */
    KASSERT(context->refCount == 0);

    ++context->refCount;
    Enable_Interrupts();
}

/*
 * If the given thread has a user context, detach it
 * and destroy it.  This is called when a thread is
 * being destroyed.
 */
void Detach_User_Context(struct Kernel_Thread* kthread)
{

    Debug ("Detach_User_Context()\n");

    struct User_Context* old = kthread->userContext;

    kthread->userContext = 0;

    if (old != 0) {
	    int refCount;

	    Disable_Interrupts();
        --old->refCount;
	    refCount = old->refCount;
	    Enable_Interrupts();

	    Debug("User context refcount == %d\n", refCount);

        if (refCount == 0)
            Destroy_User_Context(old);
    }
}

/*
 * Spawn a user process.
 * Params:
 *   program - the full path of the program executable file
 *   command - the command, including name of program and arguments
 *   stdInput - File to be Cloned as the new process's stdin
 *   stdOutput - File to be Cloned as the new process's stdout
 *   pThread - reference to Kernel_Thread pointer where a pointer to
 *     the newly created user mode thread (process) should be
 *     stored
 *   uid - the user id of the new process
 * Returns:
 *   The process id (pid) of the new process, or an error code
 *   if the process couldn't be created.  Note that this function
 *   should return ENOTFOUND if the reason for failure is that
 *   the executable file doesn't exist.
 */
int Spawn(const char *program, const char *command,
    struct File *stdInput, struct File *stdOutput,
    struct Kernel_Thread **pThread, int uid)
{
    Debug ("Spawn(): program='%s', command='%s'\n", program, command);

    /*
     * Hints:
     * - Call Read_Fully() to load the entire executable into a memory buffer
     * - Call Parse_ELF_Executable() to verify that the executable is
     *   valid, and to populate an Exe_Format data structure describing
     *   how the executable should be loaded
     * - Call Load_User_Program() to create a User_Context with the loaded
     *   program
     * - Call Start_User_Thread() with the new User_Context
     *
     * If all goes well, store the pointer to the new thread in
     * pThread and return 0.  Otherwise, return an error code.
     */
    //TODO("Spawn a process by reading an executable from a filesystem");

    int rc = EUNSPECIFIED;
    void *buf = 0;
    ulong_t fileLen;
    struct Exe_Format exeFormat;
    struct User_Context *userContext;
    struct  VFS_File_Stat stat;

    rc = Stat (program, &stat);
    if (rc < 0)  goto end;

    rc = Read_Fully (program, &buf, &fileLen);
    if (rc < 0)  goto end;

    rc = Parse_ELF_Executable (buf, fileLen, &exeFormat);
    if (rc < 0)  goto end;

    rc = Load_User_Program (buf, fileLen, &exeFormat, command, &userContext);
    if (rc < 0)  goto end;

    rc = Clone_File(stdInput,  &userContext->iob[0]);
    if (rc < 0)  goto end;

    rc = Clone_File(stdOutput, &userContext->iob[1]);
    if (rc < 0)  goto end;

//  rc = Clone_File(stdOutput, &userContext->iob[2]);
//  if (rc < 0)  goto end;

    if (stat.isSetuid)
        uid = stat.acls[0].uid;

    userContext->eUId = uid;

    *pThread = Start_User_Thread (userContext, false);
    if (!*pThread)  { rc = EUNSPECIFIED; goto end; }

    rc = (*pThread)->pid;

end:
    if (buf)  Free (buf);
    return rc;
}

/*
 * If the given thread has a User_Context,
 * switch to its memory space.
 *
 * Params:
 *   kthread - the thread that is about to execute
 *   state - saved processor registers describing the state when
 *      the thread was interrupted
 */
void Switch_To_User_Context(struct Kernel_Thread* kthread, struct Interrupt_State* state)
{
    /*
     * Hint: Before executing in user mode, you will need to call
     * the Set_Kernel_Stack_Pointer() and Switch_To_Address_Space()
     * functions.
     */
    //TODO("Switch to a new user address space, if necessary");

    struct User_Context* userContext = kthread->userContext;

    KASSERT(!Interrupts_Enabled());

    if (userContext) {
        ulong_t esp0 = ((ulong_t) kthread->stackPage) + PAGE_SIZE;
        Set_Kernel_Stack_Pointer (esp0);
        Switch_To_Address_Space (userContext);
    }
}

