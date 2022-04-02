/*
 * GeekOS system calls
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.33 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef GEEKOS_SYSCALL_H
#define GEEKOS_SYSCALL_H

#if defined(GEEKOS)



#include <geekos/kthread.h>


struct Interrupt_State;

int DestroySemaphore(int sem_id, struct Kernel_Thread* kthread);


/*
 * Function signature of a system call.
 */
typedef int (*Syscall)(struct Interrupt_State* state);

/*
 * Number of system calls implemented.
 */
extern const int g_numSyscalls;

/*
 * Table of system call handler functions.
 */
extern const Syscall g_syscallTable[];

#endif  /* defined(GEEKOS) */

#define SYSCALL "int $0x90"	 /* Assembly instruction for the system call trap. */

/*
 * System call numbers
 */
enum {
    SYS_NULL,		 /* Null (no-op) system call  */
    SYS_EXIT,		 /* Exit system call  */
    SYS_PRINTSTRING,	 /* Print string system call  */
    SYS_GETKEY,		 /* Get key system call  */
    SYS_SETATTR,	 /* Set screen attribute system call  */
    SYS_GETCURSOR,	 /* Get current cursor position */
    SYS_PUTCURSOR,	 /* Put current cursor position */
    SYS_SPAWN,		 /* Spawn process system call  */
    SYS_WAIT,		 /* Wait for child process to exit system call  */
    SYS_GETPID,		 /* Get pid (process id) system call  */
    SYS_SETSCHEDULINGPOLICY,    /* Set scheduler policy system call  */
    SYS_GETTIMEOFDAY,	 /* Get time of day system call  */
    SYS_CREATESEMAPHORE, /* Create semaphore system call  */
    SYS_P,		 /* P (acquire semaphore) system call  */
    SYS_V,		 /* V (release semaphore) system call  */
    SYS_DESTROYSEMAPHORE,  /* Destroy semaphore system call  */
    SYS_PRINTPROCESSLIST,  /* Print process list to screen  */
    SYS_PRINTSYSINFO,    /* Print system information to screen  */
    SYS_SELECTPAGINGALGORITHM,	/* set the paging rules */
    SYS_MOUNT,		 /* Mount filesystem system call  */
    SYS_OPEN,		 /* Open file system call  */
    SYS_OPENDIRECTORY,	 /* Open directory system call  */
    SYS_CLOSE,		 /* Close file system call  */
    SYS_DELETE,		 /* Delete file system call  */
    SYS_READ,		 /* Read from file system call  */
    SYS_READENTRY,	 /* Read directory entry system call  */
    SYS_WRITE,		 /* Write to file system call  */
    SYS_STAT,		 /* Stat system call  */
    SYS_FSTAT,		 /* FStat system call  */
    SYS_SEEK,		 /* Seek in file system call  */
    SYS_CREATEDIR,	 /* Create directory system call  */
    SYS_SYNC,		 /* Sync filesystems system call  */
    SYS_FORMAT,		 /* Format filesystem system call  */
    SYS_CREATEPIPE,	 /* CreatePipe system call. */
    SYS_MESSAGEQUEUE_CREATE,    /* Create message queue system call  */
    SYS_MESSAGEQUEUE_DESTROY,   /* Destroy message queue system call  */
    SYS_MESSAGEQUEUE_SEND,      /* Message queue send system call  */
    SYS_MESSAGEQUEUE_RECEIVE,   /* Message queue receive system call  */
    SYS_SBRK,            /* extend the processes heap memory */
    SYS_SET_ACL,         /* set access control list  */
    SYS_SET_SET_UID,            /* set user identification  */
    SYS_SET_EFFECTIVE_UID,      /* set effective user identification  */
    SYS_GET_UID,         /* get user identification  */
};

/*
 * Macros for convenient generation of user space
 * system call wrapper functions.
 *
 * See "src/libc/conio.c" and "src/libc/process.c" for examples of how
 * to use these macros.
 *
 * Register conventions:
 * eax - system call number [input], return value [output]
 * ebx - first argument [input]
 * ecx - second argument [input]
 * edx - third argument [input]
 * esi - fourth argument [input]
 * edi - fifth argument [input]
 */

#define SYSCALL_REGS_0
#define SYSCALL_REGS_1 , "b" (arg0)
#define SYSCALL_REGS_2 , "b" (arg0), "c" (arg1)
#define SYSCALL_REGS_3 , "b" (arg0), "c" (arg1), "d" (arg2)
#define SYSCALL_REGS_4 , "b" (arg0), "c" (arg1), "d" (arg2), "S" (arg3)
#define SYSCALL_REGS_5 , "b" (arg0), "c" (arg1), "d" (arg2), "S" (arg3), "D" (arg4)

#define DEF_SYSCALL(name,num,retType,params,argDefs,regs)		\
retType name params {							\
    int sysNum = (num), rc;						\
    argDefs								\
    __asm__ __volatile__ (SYSCALL : "=a" (rc) :"a" (sysNum) regs);	\
    return (retType) rc;						\
}

#endif  /* GEEKOS_SYSCALL_H */
