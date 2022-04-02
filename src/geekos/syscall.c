/*
 * System call handlers
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.59 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/elf.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/vfs.h>
#include <geekos/scheduler.h>
#include <geekos/sysinfo.h>
#include <geekos/mqueue.h>
#include <geekos/pipefs.h>


#ifdef DEBUG
#ifndef SYSCALL_DEBUG
#define SYSCALL_DEBUG
#endif
#endif

#ifdef DEBUG
#ifndef SEMAPHORE_DEBUG
#define SEMAPHORE_DEBUG
#endif
#endif

void Hex_Dump (void* paddr, ulong_t len);

/*
 * Null system call.
 * Does nothing except immediately return control back
 * to the interrupted user program.
 * Params:
 *  state - processor registers from user mode
 *
 * Returns:
 *   always returns the value 0 (zero)
 */
static int Sys_Null(struct Interrupt_State* state)
{
    return 0;
}

/*
 * Exit system call.
 * The interrupted user process is terminated.
 * Params:
 *   state->ebx - process exit code
 * Returns:
 *   Never returns to user mode!
 */
static int Sys_Exit(struct Interrupt_State* state)
{
    //TODO("Exit system call");
#ifdef SYSCALL_DEBUG
    Print ("Exit system call, pid=%d, rc=%d\n", g_currentThread->pid, state->ebx);
#endif
    Exit (state->ebx);
    return 0;   /* we shouldn't need this */
}

/*
 * Print a string to the console.
 * Params:
 *   state->ebx - user pointer of string to be printed
 *   state->ecx - number of characters to print
 * Returns: 0 if successful, -1 if not
 */
static int Sys_PrintString(struct Interrupt_State* state)
{
#define BUF_SIZE 512
    //TODO("PrintString system call");
#ifdef SYSCALL_DEBUG
    Print ("PrintString system call, start=%08X, length=%d\n", state->ebx, state->ecx);
#endif
    char buf[BUF_SIZE];                /* write buffer */
    int rc = EUNSPECIFIED;
    ulong_t h;                    /* helper variable */
    ulong_t s = state->ebx;            /* start address */
    ulong_t l = state->ebx + state->ecx;    /* end address */
    long x = l - s;            /* remaining number of characters to be written*/

    if (x > 0)  rc = 0;

    while (x > 0) {
       h = x <= BUF_SIZE ? x : BUF_SIZE;
       if (!Copy_From_User (&buf, s, h))  rc = EUNSPECIFIED;
       Put_Buf (buf, h);
       s += BUF_SIZE;
       x = l - s;
    }
    return rc;
#undef BUF_SIZE
}

/*
 * Get a single key press from the console.
 * Suspends the user process until a key press is available.
 * Params:
 *   state - processor registers from user mode
 * Returns: the key code
 */
static int Sys_GetKey(struct Interrupt_State* state)
{
    //TODO("GetKey system call");
#ifdef SYSCALL_DEBUG
    Print ("GetKey system call\n");
#endif
    return Wait_For_Key();
}

/*
 * Set the current text attributes.
 * Params:
 *   state->ebx - character attributes to use
 * Returns: always returns 0
 */
static int Sys_SetAttr(struct Interrupt_State* state)
{
    //TODO("SetAttr system call");
#ifdef SYSCALL_DEBUG
    Print ("SetAttr system call, attr=%08X\n", state->ebx);
#endif
    Set_Current_Attr(state->ebx);
    return 0;
}

/*
 * Get the current cursor position.
 * Params:
 *   state->ebx - pointer to user int where row value should be stored
 *   state->ecx - pointer to user int where column value should be stored
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_GetCursor(struct Interrupt_State* state)
{
    //TODO("GetCursor system call");
    int row, col;

    Get_Cursor(&row, &col);
#ifdef SYSCALL_DEBUG
    Print ("GetCursor system call, row=%d, col=%d\n", row, col);
#endif
    if ( Copy_To_User(state->ebx, &row, sizeof(int)) &&
         Copy_To_User(state->ecx, &col, sizeof(int))   )  return 0;

    return -1;
}

/*
 * Set the current cursor position.
 * Params:
 *   state->ebx - new row value
 *   state->ecx - new column value
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_PutCursor(struct Interrupt_State* state)
{
    //TODO("PutCursor system call");
    return Put_Cursor(state->ebx, state->ecx) ? 0 : -1;
}

/*
 * Create a new user process.
 * Params:
 *   state->ebx - user address of name of executable
 *   state->ecx - length of executable name
 *   state->edx - user address of command string
 *   state->esi - length of command string
 *   state->edi == stdin file descriptor in low 16 bits, stdout file descriptor in high 16 bits
 * Returns: pid of process if successful, error code (< 0) otherwise
 */
static int Sys_Spawn(struct Interrupt_State* state)
{
    /*
     * NOTE: here is how to get the file descriptors for
     * the child's stdin and stdout.
     * int stdinFd = (int) (state->edi & 0xffff);
     * int stdoutFd = (int) (state->edi >> 16);
     */
    //TODO("Spawn system call");

    struct Kernel_Thread *t;
    char *name = 0;
    char *cmd = 0;
    int pid = EINVALID;
    int stdinFd = (int) (state->edi & 0xffff);
    int stdoutFd = (int) (state->edi >> 16);

    if (stdinFd >= USER_MAX_FILES || stdoutFd >= USER_MAX_FILES)
        return EINVALID;

    struct File *stdInput = g_currentThread->userContext->iob[stdinFd];
    struct File *stdOutput = g_currentThread->userContext->iob[stdoutFd];
    int uid = g_currentThread->userContext->eUId;

    KASSERT(!Interrupts_Enabled());

#ifdef SYSCALL_DEBUG
    Print ("Sys_Spawn: name:ebx=0x%08x ecx=%d. cmd string: edx=0x%08x esi=%d\n",
       state->ebx, state->ecx, state->edx, state->esi);
#endif

    name = Malloc (state->ecx + 1);  if (!name)  goto last;
    cmd = Malloc (state->esi + 1);   if (!cmd)  goto last;

    if ( !Copy_From_User(name, state->ebx, state->ecx) )  goto last;
    if ( !Copy_From_User(cmd,  state->edx, state->esi) )  goto last;

    name[state->ecx] = '\0';
    cmd[state->esi] = '\0';
#ifdef SYSCALL_DEBUG
    Print ("Spawn system call, name='%s', cmd='%s', uid=%d\n", name, cmd, uid);
#endif

    Enable_Interrupts();
    pid = Spawn (name, cmd, stdInput, stdOutput, &t, uid);
    Disable_Interrupts();

last:
    if (name)  Free (name);
    if (cmd)   Free (cmd);
    return pid;
}

/*
 * Wait for a process to exit.
 * Params:
 *   state->ebx - pid of process to wait for
 * Returns: the exit code of the process,
 *   or error code (< 0) on error
 */
static int Sys_Wait(struct Interrupt_State* state)
{
    // TODO("Wait system call");
#ifdef SYSCALL_DEBUG
    Print ("Wait system call, pid='%d'\n", state->ebx);
#endif
    struct Kernel_Thread *t = Lookup_Thread (state->ebx);

    if (!t)   return EUNSPECIFIED;

    Enable_Interrupts();
    int rc = Join (t);
    Disable_Interrupts();
    return rc;
}

/*
 * print list of all processes on screen
 * Params:
 *   state - processor registers from user mode
 * Returns: 1
 */
static int Sys_PrintProcessList(struct Interrupt_State* state)
{
    return PrintProcessList();
}

/*
 * print system information
 * Params:
 *   state->ebx - flags for printing
 * Returns: 0 on success
 *   or error code (< 0) on error
 */
static int Sys_PrintSysInfo(struct Interrupt_State* state)
{
    return PrintSystemInfo(state->ebx);
}


/*
 * select paging algorithm
 * Params:
 *   state->ebx - the paging algorithm
 * Returns: 0 on success
 *   or error code (< 0) on error
 */
static int Sys_SelectPagingAlgorithm(struct Interrupt_State* state)
{
    return SelectPagingAlgorithm(state->ebx);
}


/*
 * Get pid (process id) of current thread.
 * Params:
 *   state - processor registers from user mode
 * Returns: the pid of the current thread
 */
static int Sys_GetPID(struct Interrupt_State* state)
{
    // TODO("GetPID system call");
#ifdef SYSCALL_DEBUG
    Print ("GetPID system call, pid='%d'\n", g_currentThread->pid);
#endif
    return g_currentThread->pid;
}

/*
 * Set the scheduling policy.
 * Params:
 *   state->ebx - policy,
 *   state->ecx - number of ticks in quantum
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_SetSchedulingPolicy(struct Interrupt_State* state)
{
    //TODO("SetSchedulingPolicy system call");

    uint_t  policy;
    uint_t  quantum;

    policy = state->ebx;
    quantum = state->ecx;

    KASSERT(!Interrupts_Enabled());

    return Switch2SchedulingPolicy(policy, quantum);
}

/*
 * Get the time of day.
 * Params:
 *   state - processor registers from user mode
 *
 * Returns: value of the g_numTicks global variable
 */
static int Sys_GetTimeOfDay(struct Interrupt_State* state)
{
    //TODO("GetTimeOfDay system call");
    return g_numTicks;
}

void PrintAllSemaphores()
{
    int k;
    struct Kernel_Thread* kthread;
    struct ThreadsSemaphore *kthread_sem;    

    Print("List of Semaphores\nSemID RegUsr SemCount SemName\n");
    for (k=0; k < MAX_SEM; k++)
    {
        if (g_semaphores[k].sem_id) 
        {
            Print("%5d %6d %8d %s\n",g_semaphores[k].sem_id, g_semaphores[k].registered_users, 
                g_semaphores[k].sem_count, g_semaphores[k].sem_name);
        }
    }

    Print("looping through threads and looking for assigned semaphores\n");
    Print("SemQAdr ProcessId SemId\n");
    kthread = Get_FirstOfAllThreads();
    while (kthread)
    {
        kthread_sem = Get_Front_Of_Thread_Semaphore_List(&(kthread->SemaphoreList));
        while (kthread_sem)
        {
            Print("%7X %9d %5d\n",(unsigned int)&(kthread->SemaphoreList),
                kthread->pid, kthread_sem->semaphore->sem_id);    
            kthread_sem = Get_Next_In_Thread_Semaphore_List(kthread_sem);
        }
        kthread=Get_NextOfAllThreads(kthread);
    }
    Print("\n");
}

/*
 * Create a semaphore.
 * Params:
 *   state->ebx - user address of name of semaphore
 *   state->ecx - length of semaphore name
 *   state->edx - initial semaphore count
 * Returns: the global semaphore id
 */
static int Sys_CreateSemaphore(struct Interrupt_State* state)
{
    //TODO("CreateSemaphore system call");
    
    int k = 0;
    uint_t initial_count = state->edx;
    char sem_name[MAX_SEM_NAME+1];
    
#ifdef SEMAPHORE_DEBUG
    Print("before creating semaphore for thread %d\n",Get_Current()->pid);
    PrintAllSemaphores();
#endif
    
    k = state->ecx < MAX_SEM_NAME ? state->ecx : MAX_SEM_NAME;
    if (!Copy_From_User (sem_name, state->ebx, k))
        return EUNSPECIFIED;
    sem_name[k] = '\0';

#ifdef SEMAPHORE_DEBUG
    Print("Semaphore Name: %s\n", sem_name);
#endif

    /* determine if the semaphore already exists */
    for (k=0; k < MAX_SEM; k++)
        if (g_semaphores[k].sem_id && !strcmp(g_semaphores[k].sem_name, sem_name)) {
            bool iflag = Begin_Int_Atomic();
            g_semaphores[k].registered_users++;
            
            // add semaphore to threads list of available semaphores
            struct ThreadsSemaphore* newSemaphore =
                (struct ThreadsSemaphore*) Malloc(sizeof(struct ThreadsSemaphore));
            newSemaphore->semaphore = &g_semaphores[k];
            Add_To_Back_Of_Thread_Semaphore_List(&(Get_Current()->SemaphoreList), newSemaphore);
            End_Int_Atomic(iflag);

#ifdef SEMAPHORE_DEBUG
            Print("Found semaphore '%s': id=%d, and added for thread %d\n", sem_name,
                g_semaphores[k].sem_id, Get_Current()->pid);
            Print("after creating semaphore\n");
            PrintAllSemaphores();
#endif
            return g_semaphores[k].sem_id;
        }

    /* the semaphore doesn't exist yet, so we create a new one */
    bool iflag = Begin_Int_Atomic();
    for (k=0; k < MAX_SEM; k++)
        if (!g_semaphores[k].sem_id) {        /* is this a free entry? */
            g_semaphores[k].sem_id = k+1;
            g_semaphores[k].registered_users = 1;
            g_semaphores[k].sem_count = initial_count;
            strcpy(g_semaphores[k].sem_name, sem_name);
            // add semaphore to threads list of available semaphores
            struct ThreadsSemaphore* newSemaphore =
                (struct ThreadsSemaphore*) Malloc(sizeof(struct ThreadsSemaphore));
            newSemaphore->semaphore = &g_semaphores[k];
            Add_To_Back_Of_Thread_Semaphore_List(&(Get_Current()->SemaphoreList), newSemaphore);
            End_Int_Atomic(iflag);

#ifdef SEMAPHORE_DEBUG
            Print("Created semaphore '%s', id=%d and added for thread %d, ctr=%d\n", sem_name,
                k+1, Get_Current()->pid, initial_count);
#endif
#ifdef SEMAPHORE_DEBUG
            Print("after creating semaphore of thread %d\n",Get_Current()->pid);
            PrintAllSemaphores();
#endif
            return g_semaphores[k].sem_id;
        }
    End_Int_Atomic(iflag);

    return EUNSPECIFIED; /* all semapore entries are already used */
}


/*
 * Acquire a semaphore.
 * Assume that the process has permission to access the semaphore,
 * the call will block until the semaphore count is >= 0.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_P(struct Interrupt_State* state)
{
    //TODO("P (semaphore acquire) system call");

    uint_t id = state->ebx;
    if (id < 1 || id > MAX_SEM)
        return EUNSPECIFIED;    /* invalid semaphore id */

#ifdef SEMAPHORE_DEBUG
    Print("before P(%d) for thread %d\n",id,Get_Current()->pid);
    PrintAllSemaphores();
#endif

    struct Semaphore *sem = g_semaphores + id - 1;
    
    // check if process has permission to use this semaphore
    struct ThreadsSemaphore *kthread_sem =
        Get_Front_Of_Thread_Semaphore_List(&(Get_Current()->SemaphoreList));

    while (kthread_sem  &&  kthread_sem->semaphore->sem_id != id)
        kthread_sem = Get_Next_In_Thread_Semaphore_List(kthread_sem);

    if (!kthread_sem)
        return EUNSPECIFIED;    /* not registered for semaphore */
    
    while (sem->sem_count < 0)  Wait(&sem->semQueue);
        
    bool iflag = Begin_Int_Atomic();
    sem->sem_count--;
    End_Int_Atomic(iflag);

#ifdef SEMAPHORE_DEBUG
    Print("after P(%d) for thread %d\n",id,Get_Current()->pid);
    PrintAllSemaphores();
#endif

    return 0;
}

/*
 * Release a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_V(struct Interrupt_State* state)
{
    //TODO("V (semaphore release) system call");
    
    uint_t id = state->ebx;
    if (id < 1 || id > MAX_SEM)
        return EUNSPECIFIED;    /* invalid semaphore id */
#ifdef SEMAPHORE_DEBUG
    Print("before V(%d) for thread %d\n",id ,Get_Current()->pid);
    PrintAllSemaphores();
#endif

    struct Semaphore *sem = g_semaphores + id - 1;
        
    // check if process has permission to use this semaphore
    struct ThreadsSemaphore *kthread_sem =
        Get_Front_Of_Thread_Semaphore_List(&(Get_Current()->SemaphoreList));

    while (kthread_sem  &&  kthread_sem->semaphore->sem_id != id)
        kthread_sem = Get_Next_In_Thread_Semaphore_List(kthread_sem);

    if (!kthread_sem)
        return EUNSPECIFIED;    /* not registered for semaphore */
    
    bool iflag = Begin_Int_Atomic();
    sem->sem_count++;
    End_Int_Atomic(iflag);
    if (sem->sem_count <= 0)
        Wake_Up_One(&sem->semQueue);

#ifdef SEMAPHORE_DEBUG
    Print("after V(%d) for thread %d\n", id, Get_Current()->pid);
    PrintAllSemaphores();
#endif

    return 0;
}

/*
 * Destroy a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_DestroySemaphore(struct Interrupt_State* state)
{
    //TODO("DestroySemaphore system call");
    
    uint_t id = state->ebx;
    
    return DestroySemaphore(id, Get_Current());
}


int DestroySemaphore(int sem_id, struct Kernel_Thread* kthread)
{
    int id=sem_id;
    
    if (id < 1 || id > MAX_SEM)
        return EUNSPECIFIED;    /* invalid semaphore id */

#ifdef SEMAPHORE_DEBUG
    Print("before destroy semaphore %d of thread %d\n",id,kthread->pid);
    PrintAllSemaphores();
#endif

    struct Semaphore *sem = g_semaphores + id - 1;
#ifdef SEMAPHORE_DEBUG
    Print("Trying to Destroy: '%s', id=%d, users=%d\n", 
        sem->sem_name, id, sem->registered_users);
#endif

    // check if process has permission to use this semaphore
    struct ThreadsSemaphore *kthread_sem =
        Get_Front_Of_Thread_Semaphore_List(&(kthread->SemaphoreList));

    while (kthread_sem  &&  kthread_sem->semaphore->sem_id != id)
        kthread_sem = Get_Next_In_Thread_Semaphore_List(kthread_sem);

    if (!kthread_sem)
        return EUNSPECIFIED;    /* not registered for semaphore */
    
    bool iflag = Begin_Int_Atomic();
    sem->registered_users--;    /* unregister current user */
    
    // remove semaphore from the threads list of available semaphores
    /* remove semaphore from current_thread */
    Remove_From_Thread_Semaphore_List(&(kthread->SemaphoreList), kthread_sem);
    Free(kthread_sem);    // speicher dieser Semaphore-Envelope wieder freigeben
    
    if (!sem->registered_users) {    /* no more users left? */
        sem->sem_id = 0;    /* free semaphore entry */
        Clear_Thread_Queue(&sem->semQueue);
    }

    End_Int_Atomic(iflag);    /*  Check with Clear_Thread_Queue()! */

#ifdef SEMAPHORE_DEBUG
    Print("After destroy semaphore %d of thread %d\n", id, kthread->pid);
    PrintAllSemaphores();
#endif    
    
    return 0;
}


/*
 * Mount a filesystem.
 * Params:
 * state->ebx - contains a pointer to the Mount_Syscall_Args structure
 *   which contains the block device name, mount prefix,
 *   and filesystem type
 *
 * Returns:
 *   0 if successful, error code if unsuccessful
 */
static int Sys_Mount(struct Interrupt_State *state)
{
    //TODO("Mount system call");
    int rc = 0;
    struct VFS_Mount_Request *args = 0;

    /* Allocate space for VFS_Mount_Request struct. */
    args = (struct VFS_Mount_Request *) Malloc(sizeof(struct VFS_Mount_Request));
    if (args== 0) return ENOMEM;


    /* Copy the mount arguments structure from user space. */
    if (!Copy_From_User(args, state->ebx, sizeof(struct VFS_Mount_Request))) 
    {
        rc = EINVALID;
        goto done;
    }

    /*
     * Hint: use devname, prefix, and fstype from the args structure
     * and invoke the Mount() VFS function.  You will need to check
     * to make sure they are correctly nul-terminated.
     */
    
    if (   (strlen(args->devname) >= BLOCKDEV_MAX_NAME_LEN)
        || (strlen(args->fstype)  >= VFS_MAX_FS_NAME_LEN)
        || (strlen(args->prefix)  >= VFS_MAX_PATH_LEN) )
    {
        rc = -1;
        goto done;
    }
    
#ifdef FS_DEBUG
    Print("| %s | %s | %s |\n", args->devname, args->fstype, args->prefix);
#endif
    
    Enable_Interrupts();
    rc = Mount(args->devname, args->prefix, args->fstype);
    Disable_Interrupts();

#ifdef FS_DEBUG
    Print("Mount returned: %d\n",rc);
#endif
    
done:
    if (args)  Free(args);
    return rc;
}

/*
 * Open a file.
 * Params:
 *   state->ebx - address of user string containing path of file to open
 *   state->ecx - length of path
 *   state->edx - file mode flags
 *
 * Returns: a file descriptor (>= 0) if successful,
 *   or an error code (< 0) if unsuccessful
 */
static int Sys_Open(struct Interrupt_State *state)
{
    //TODO("Open system call");
    char   *path = 0;
    int    rc = -1,  mode = state->edx;
    int    pathLen = state->ecx;
    struct File *ptr = 0;
    int    i = 0,  fileNum = -1;
    
#ifdef FS_DEBUG
    Print("Sys_Open: length of path: %d\n",pathLen);
    Print("Sys_Open: mode: %d\n",mode);
#endif
    
    path = Malloc (pathLen+1);

    if (!Copy_From_User(path, state->ebx, pathLen)) {
        rc = EINVALID;
        goto done;
    }
    path[pathLen] = '\0';

#ifdef FS_DEBUG
    Print("Sys_Open: path: %s\n",path);
#endif
    
    ptr = Malloc(sizeof(struct File));
    if (!ptr) {
        rc = ENOMEM;
        goto done;
    }
    
    Enable_Interrupts();
    rc = Open(path, mode, &ptr);
    Disable_Interrupts();
    
    if (rc < 0)  goto done;
    
    for (i = 0; i < USER_MAX_FILES; i++) {
        if (g_currentThread->userContext->iob[i] == 0) {
            g_currentThread->userContext->iob[i] = ptr;
            fileNum = i;
            break;
        }
    }
    if (fileNum < 0)  rc = EUSRMAXFILES;
    
done:
//  Hex_Dump (g_currentThread->userContext->iob, sizeof(struct File*) * USER_MAX_FILES);
//  Print ("Sys_Open: fd=%d path=%s\n", fileNum, path);
    if (path)  Free(path);
    if (rc >= 0)
        return fileNum;

    if (ptr)  Free(ptr);
    return rc;
}

/*
 * Open a directory.
 * Params:
 *   state->ebx - address of user string containing path of directory to open
 *   state->ecx - length of path
 *
 * Returns: a file descriptor (>= 0) if successful,
 *   or an error code (< 0) if unsuccessful
 */
static int Sys_OpenDirectory(struct Interrupt_State *state)
{
    //TODO("Open directory system call");
    char   *path = 0;
    int    rc = -1,  i = 0,  fileNum = -1;
    int    pathLen = state->ecx;
    struct File *ptr = 0;
    
#ifdef FS_DEBUG
    Print("Sys_OpenDirectory: length of path: %d\n",pathLen);
#endif
    
    path = Malloc(pathLen+1);

    if (!Copy_From_User(path, state->ebx, pathLen)) {
        rc = EINVALID;
        goto done;
    }
    path[pathLen] = '\0';

#ifdef FS_DEBUG
    Print("Sys_OpenDirectory: path: %s\n",path);
#endif
    
    ptr = Malloc(sizeof(struct File));
    if (!ptr) {
        rc = ENOMEM;
        goto done;
    }
    
    Enable_Interrupts();
    rc = Open_Directory(path, &ptr);
    Disable_Interrupts();
    
    if (rc < 0)  goto done;
    
    for (i=0; i < USER_MAX_FILES; i++) {
        if (g_currentThread->userContext->iob[i] == 0) {
            g_currentThread->userContext->iob[i] = ptr;
            fileNum=i;
            break;
        }
    }
    if (fileNum < 0)  rc = EUSRMAXFILES;
    
done:
//  Hex_Dump (g_currentThread->userContext->iob, sizeof(struct File*) * USER_MAX_FILES);
//  Print("Sys_OpenDirectory: fd=%d path=%s\n", fileNum, path);
    if (path)  Free (path);
    if (rc >= 0)
        return fileNum;

    if (ptr)  Free(ptr);
    return rc;
}

/*
 * Close an open file or directory.
 * Params:
 *   state->ebx - file descriptor of the open file or directory
 * Returns: 0 if successful, or an error code (< 0) if unsuccessful
 */
static int Sys_Close(struct Interrupt_State *state)
{
    int rc = 0;
    struct File *file;
    int fileNum = state->ebx;
    
    if (fileNum < 0 || fileNum >= USER_MAX_FILES)
        return EINVALID;
    
    file = (struct File*) g_currentThread->userContext->iob[fileNum];
#ifdef FS_DEBUG
    Print("Sys_Close pid=%d fd=%d %p\n", g_currentThread->pid, state->ebx, file);
#endif

    if (!file)  return EIO;

    Enable_Interrupts();
    rc = Close(file);
    Disable_Interrupts();

    g_currentThread->userContext->iob[fileNum] = 0;
    
//  Hex_Dump (g_currentThread->userContext->iob, sizeof(struct File*) * USER_MAX_FILES);
    return rc;
}

/*
 * Delete a file.
 * Params:
 *   state->ebx - address of user string containing path to delete
 *   state->ecx - length of path
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Delete(struct Interrupt_State *state)
{
    //TODO("Delete system call");
    char    *path=0;
    int     rc=0;
    int        pathLen = state->ecx;

#ifdef FS_DEBUG
    Print("Sys_Delete: length of path: %d\n",pathLen);
#endif

    path = Malloc(pathLen+1);

    if (!Copy_From_User(path, state->ebx, pathLen)) {
        rc = EINVALID;
        goto done;
    }
    path[pathLen] = '\0';

#ifdef FS_DEBUG
    Print("Sys_Delete: path: %s\n",path);
#endif

    Enable_Interrupts();
    rc = Delete(path);
    Disable_Interrupts();

done:
    if (path)  Free(path);
    return rc;
}

/*
 * Read from an open file.
 * Params:
 *   state->ebx - file descriptor to read from
 *   state->ecx - user address of buffer to read into
 *   state->edx - number of bytes to read
 *
 * Returns: number of bytes read, 0 if end of file,
 *   or error code (< 0) on error
 */
static int Sys_Read(struct Interrupt_State *state)
{
    //TODO("Read system call");
    int rc=0;
    if (state->ebx < 0 || state->ebx >= USER_MAX_FILES)
        return EINVALID;

    struct File* pFile = g_currentThread->userContext->iob[state->ebx];
    ulong_t numBytes = (ulong_t) state->edx;
    void *p_buff = 0;
    
    p_buff=Malloc(numBytes);
    if (p_buff == 0) {
        rc = ENOMEM;
        goto done;
    }
    
    Enable_Interrupts();
    rc = Read(pFile, p_buff,numBytes);
    Disable_Interrupts();
    
    if (!Copy_To_User(state->ecx, p_buff, numBytes)) {
        rc = EINVALID;
    }
    
done:
    if (p_buff)  Free(p_buff);
    return rc;
}

/*
 * Read a directory entry from an open directory handle.
 * Params:
 *   state->ebx - file descriptor of the directory
 *   state->ecx - user address of struct VFS_Dir_Entry to copy entry into
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_ReadEntry(struct Interrupt_State *state)
{
    //TODO("ReadEntry system call");
    if (state->ebx < 0 || state->ebx >= USER_MAX_FILES)
        return EINVALID;

    int    rc = 0;
    struct File *file;
    struct VFS_Dir_Entry dirEntry;
    
    /* file handle already in kernel space, so no copy needed */
    /*
    if (!Copy_From_User(&file, state->ebx, sizeof(struct File))) 
    {
        rc = EINVALID;
        goto done;
    }
    */
    file = (struct File*)g_currentThread->userContext->iob[state->ebx];
        
    
    Enable_Interrupts();
    rc = Read_Entry(file, &dirEntry);
    Disable_Interrupts();
    
    if (!Copy_To_User(state->ecx, &dirEntry, sizeof(struct VFS_Dir_Entry))) {
        rc = EINVALID;
    }
    
    return rc;
}

/*
 * Write to an open file.
 * Params:
 *   state->ebx - file descriptor to write to
 *   state->ecx - user address of buffer get data to write from
 *   state->edx - number of bytes to write
 *
 * Returns: number of bytes written,
 *   or error code (< 0) on error
 */
static int Sys_Write(struct Interrupt_State *state)
{
    //TODO("Write system call");
    if (state->ebx < 0 || state->ebx >= USER_MAX_FILES)
        return EINVALID;

    int rc=0;
    struct File *pFile = g_currentThread->userContext->iob[state->ebx];
    void*    p_buff;
    ulong_t bufflen=state->edx;

#if 0    
#ifdef FS_DEBUG
    Print("Sys_Write: file descriptor %08x\n", (uint_t) pFile);
    Print("Sys_Write: bytes to write %ld\n",bufflen);
#endif
#endif

    if (!bufflen)
        return EINVALID;
    
    p_buff = Malloc(bufflen);
    if (p_buff == 0) {
        rc = ENOMEM;
        goto finish;
    }
    memset(p_buff,'\0',bufflen);

    if (!Copy_From_User(p_buff, state->ecx, bufflen)) {
        rc = EINVALID;
        goto finish;
    }
    
    Enable_Interrupts();
    rc = Write(pFile, p_buff, bufflen);
    Disable_Interrupts();
    
finish:
    if (p_buff)  Free(p_buff);
    return rc;
}

/*
 * Get file metadata.
 * Params:
 *   state->ebx - address of user string containing path of file
 *   state->ecx - length of path
 *   state->edx - user address of struct VFS_File_Stat object to store metadata in
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Stat(struct Interrupt_State *state)
{
    // TODO("Stat system call");
    int     rc = 0;
    char    *path = 0;
    int     pathLen = state->ecx;
    struct  VFS_File_Stat stat;
    
#ifdef FS_DEBUG
    Print("sys_Stat: length of path: %d\n",pathLen);
#endif
    
    path = Malloc(pathLen+1);

    /* Copy the mount arguments structure from user space. */
    if (!Copy_From_User(path, state->ebx, pathLen)) {
        rc = EINVALID;
        goto done;
    }
    path[pathLen] = '\0';

#ifdef FS_DEBUG
    Print("sys_Stat: path: %s\n",path);
#endif
    /*
    if (!Copy_From_User(&stat, state->edx, sizeof(struct VFS_File_Stat))) 
    {
        rc = EINVALID;
        goto done;
    }
    */
    
    Enable_Interrupts();
    rc = Stat(path, &stat);
    Disable_Interrupts();
    
    if (!Copy_To_User(state->edx, &stat, sizeof(struct VFS_File_Stat))) {
        rc = EINVALID;
        goto done;
    }
    
done:
    if (path)  Free(path);
    return rc;
}

/*
 * Get metadata of an open file.
 * Params:
 *   state->ebx - file descriptor to get metadata for
 *   state->ecx - user address of struct VFS_File_Stat object to store metadata in
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_FStat(struct Interrupt_State *state)
{
    //TODO("FStat system call");
    if (state->ebx < 0 || state->ebx >= USER_MAX_FILES)
        return EINVALID;

    int rc = 0;
    struct File *pFile= (struct File*)g_currentThread->userContext->iob[state->ebx];
    struct VFS_File_Stat fileStat;
    
    Enable_Interrupts();
    rc = FStat(pFile, &fileStat);
    Disable_Interrupts();
    
    if (!Copy_To_User(state->ecx, &fileStat, sizeof(struct VFS_File_Stat))) {
        rc = EINVALID;
    }
    
    return rc;
}

/*
 * Change the access position in a file
 * Params:
 *   state->ebx - file descriptor 
 *   state->ecx - position in file
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Seek(struct Interrupt_State *state)
{
    //TODO("Seek system call");
    if (state->ebx < 0 || state->ebx >= USER_MAX_FILES)
        return EINVALID;

    int rc = 0;
    struct File* file = (struct File*) g_currentThread->userContext->iob[state->ebx];
    ulong_t offset=state->ecx;
    
    rc = Seek(file,offset);
    
    return rc;
}

/*
 * Create directory
 * Params:
 *   state->ebx - address of user string containing path of new directory
 *   state->ecx - length of path
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_CreateDir(struct Interrupt_State *state)
{
    //TODO("CreateDir system call");
    int     rc = 0;
    char    *dirName=0;
    int     dirLen = state->ecx;
    
#ifdef FS_DEBUG
    Print("sys_CreateDir: lenght of directory: %d\n",dirLen);
#endif
    
    dirName = Malloc(dirLen+1);

    /* Copy the mount arguments structure from user space. */
    if (!Copy_From_User(dirName, state->ebx, dirLen)) {
        rc = EINVALID;
        goto done;
    }
    dirName[dirLen]='\0';
    
    Enable_Interrupts();
    rc = Create_Directory(dirName);
    Disable_Interrupts();
    
done:
    if (dirName)  Free(dirName);
    return rc;
}

/*
 * Flush filesystem buffers
 * Params: none 
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Sync(struct Interrupt_State *state)
{
    //TODO("Sync system call");
    int rc=0;
    
    Enable_Interrupts();
    Sync();
    Disable_Interrupts();
    
    return rc;
}
/*
 * Format a device
 * Params:
 *   state->ebx - address of user string containing device to format
 *   state->ecx - length of device name string
 *   state->edx - address of user string containing filesystem type 
 *   state->esi - length of filesystem type string

 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_Format(struct Interrupt_State *state)
{
    //TODO("Format system call");
    int        rc=0;
    char*    device=0;
    char*    fstype=0;
    
    device = Malloc(state->ecx + 1);
    fstype = Malloc(state->esi + 1);
    
    if (!Copy_From_User (device, state->ebx, state->ecx))
        return EUNSPECIFIED;
    
    if (!Copy_From_User (fstype, state->edx, state->esi))
        return EUNSPECIFIED;
    
    device[state->ecx]='\0';
    fstype[state->esi]='\0';
    
#ifdef FS_DEBUG
    Print("sys_Format: Device: %s\n",device);
    Print("sys_Format: FSType: %s\n",fstype);
#endif
    
    Enable_Interrupts();
    rc = Format(device, fstype);
    Disable_Interrupts();
    
#ifdef FS_DEBUG
    Print("sys_Format: Format returend: %d\n",rc);
#endif
    
    if (device)  Free(device);
    if (fstype)  Free(fstype);
    
    return rc;
}

/*
 * Create a Pipe
 * Params:
 *   state->ebx - address of read file descriptor number in user space
 *   state->ecx - address of write file descriptor number in user space
 *
 * Returns: 0 if successful,
 *   or an error code (< 0) if unsuccessful
 */
static int Sys_CreatePipe(struct Interrupt_State *state)
{
    //TODO("CreatePipe system call");

    if (!state->ebx || !state->ecx)
        return EINVALID;

    int fds[2], i, k;

    k = 0;

    for (i = 0; i < USER_MAX_FILES && k < 2; i++)
        if (!g_currentThread->userContext->iob[i])
            fds[k++] = i;

#ifdef PIPE_DEBUG
    Print ("Sys_CreatePipe: pid=%d, rd=%d, wr=%d\n", g_currentThread->pid, fds[0], fds[1]);
#endif

    if (k < 2)
        return EMFILE;

    struct File *pRead = 0;
    struct File *pWrite = 0;
    int rc;

    rc = Create_Pipe (&pRead, &pWrite);
    if (rc < 0)
        return rc;

#ifdef PIPE_DEBUG
    Print ("Sys_CreatePipe: pid=%d, rdf=%p, wrf=%p\n", g_currentThread->pid, pRead, pWrite);
#endif

    g_currentThread->userContext->iob[fds[0]] = pRead;
    g_currentThread->userContext->iob[fds[1]] = pWrite;

    if ( !Copy_To_User(state->ebx, fds  , sizeof(int)) ||
         !Copy_To_User(state->ecx, fds+1, sizeof(int)))
    {
       Close (pRead);
       Close (pWrite);
       return EUNSPECIFIED;
    }
   
    return 0;
}

/*
 * Create a Message Queue
 * Params:
 *   state->ebx - user address of name of MQ
 *   state->ecx - length of MQ name
 *   state->edx - MQ buffer size
 *
 * Returns: the global MQ id
 * It creates the MQ if it doesn't exist, and returns the already-existing id if the MQ exists.
 */
static int Sys_MessageQueueCreate(struct Interrupt_State* state)
{
    //TODO("MessageQueueCreate system call");
    if ( state->ecx <= 0 )
        return EINVALID;

    if (state->edx > MQUEUE_MAX_SIZE)
        return EINVALID;

    char* mq_name = (char*) Malloc (state->ecx+1);   /* freed in MQ_Destroy ! */
    mq_name[state->ecx] = '\0';

    if ( !mq_name )
        return ENOMEM;

    if ( !Copy_From_User(mq_name, state->ebx, state->ecx) ) {
        Free (mq_name);
        return EINVALID;
    }

    return MQ_Create (mq_name, state->edx);
}

/*
 * Destroys a Message Queue
 * Params:
 *   state->ebx - the MQ id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 * Will destroy the MQ for this thread only. You need a ref count.
 * If the ref count gets to 0, destroys the MQ completely.
 */

static int Sys_MessageQueueDestroy(struct Interrupt_State* state)
{
    //TODO("MessageQueueDestroy system call");
    return MQ_Destroy (state->ebx);
}

/*
 * Write to a Message Queue
 * Params:
 *   state->ebx - the MQ id
 *   state->ecx - user address of buffer
 *   state->edx - length of message 
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 * Blocks is the buffer is full.
 */
static int Sys_MessageQueueSend(struct Interrupt_State* state)
{
    //TODO("MessageQueueSend system call");

    if (state->edx <= 0 || state->edx > MESSAGE_MAX_SIZE || !state->ecx)
        return EINVALID;

    struct Msg* msg;   
    msg = (struct Msg*) Malloc (sizeof(*msg));
    if (!msg)
        return ENOMEM;

    msg->data = (void*) Malloc (state->edx);
    msg->size = state->edx;

    if (!msg->data) {
        Free (msg);
        return ENOMEM;
    }

    if ( !Copy_From_User(msg->data, state->ecx, state->edx) ) {
        Free (msg->data);
        Free (msg);
        return EUNSPECIFIED;
    }

    return MQ_Send (state->ebx, msg);
}

/*
 * Read from a Message Queue
 * Params:
 *   state->ebx - the MQ id
 *   state->ecx - user address of buffer
 *   state->edx - length of message 
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 * Blocks if no data is available.
 */
static int Sys_MessageQueueReceive(struct Interrupt_State* state)
{
    //TODO("MessageQueueReceive system call");
    if (state->edx <= 0 || !state->ecx)
        return EINVALID;

    struct Msg *msg = MQ_Receive (state->ebx);
    int rc = 0;

    if (!msg)
        return EUNSPECIFIED;

    if (!Copy_To_User (state->ecx, msg->data, state->edx > msg->size? msg->size : state->edx))
        rc = EUNSPECIFIED;

    Free (msg->data);
    Free (msg);
    return rc;
}


/*
 * Extend the heap space of a process
 * Params:
 *   state->ebx - the requested extension in bytes
 *
 * Returns: the user space pointer to a newly aquired memory block,
 *          0 if not successful
 */
static int Sys_SBrk(struct Interrupt_State *state)
{
    //TODO("Sys_SBrk system call");
    ulong_t r = g_currentThread->userContext->size;
    g_currentThread->userContext->size += state->ebx;

#ifdef SYSCALL_DEBUG
    Print("Sys_SBrk: increase %d, addr=0x%lx\n", state->ebx, r);
#endif
    return r;
}

/*
 * Set Access Control List
 * Params:
 *   state->ebx - a file name
 *   state->ecx - length of the file name
 *   state->edx - user id
 *   state->esi - permissions
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 *          
 */
int Sys_SetAcl( struct Interrupt_State* state )
{
    //TODO("Sys_SetAcl system call");
    if (!state->ebx || !state->ecx)
	return EINVALID;
    
    KASSERT(g_currentThread->userContext);

    char* name = Malloc (state->ecx+1);
    if (!name)
        return ENOMEM;

    int rc;

    if ( !Copy_From_User (name, state->ebx, state->ecx) ) {
	rc = EUNSPECIFIED;
        goto done;
    }
    name[state->ecx] = '\0';

    Enable_Interrupts();
    rc = SetAcl(name, state->edx, state->esi);
    Disable_Interrupts();

done:
    Free (name);
    return rc;
}

/*
 * Set Set User Identification
 * Params:
 *   state->ebx - a file name
 *   state->ecx - length of the file name
 *   state->edx - set UID flag
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
int Sys_SetSetUid( struct Interrupt_State* state )
{
    //TODO("Sys_SetSetUid system call");
    if (!state->ebx || !state->ecx)
        return EINVALID;

    char* name = Malloc (state->ecx+1);
    if (!name)
        return ENOMEM;

    int rc;

    if ( !Copy_From_User (name, state->ebx, state->ecx) ) {
	rc = EUNSPECIFIED;
        goto done;
    }
    name[state->ecx] = '\0';

    Enable_Interrupts();
    rc = SetSetUid (name, state->edx);
    Disable_Interrupts();

done:
    Free (name);
    return rc;
}

/*
 * Set Effective User Identification
 * Params:
 *   state->ebx - 
 *
 * Returns: 
 *          
 */
int Sys_SetEffectiveUid( struct Interrupt_State* state )
{
    //TODO("Sys_SetEffectiveUid system call");
    int uid = state->ebx;
    struct User_Context *u = g_currentThread->userContext;
    KASSERT(u);

    if (u->eUId && u->eUId != uid)
        return EACCESS;
    
    u->eUId = uid;
    return 0;
}

/*
 * Get User Identification
 * Params:
 *   state->ebx - 
 *
 * Returns: 
 *          
 */
int Sys_GetUid( struct Interrupt_State* state )
{
    //TODO("Sys_GetUid system call");
    struct User_Context *u = g_currentThread->userContext;
    KASSERT(u);
    return u->eUId;
}


/*
 * Global table of system call handler functions.
 */
const Syscall g_syscallTable[] = {
    Sys_Null,
    Sys_Exit,
    Sys_PrintString,
    Sys_GetKey,
    Sys_SetAttr,
    Sys_GetCursor,
    Sys_PutCursor,
    Sys_Spawn,
    Sys_Wait,
    Sys_GetPID,
    /* Scheduling and semaphore system calls. */
    Sys_SetSchedulingPolicy,
    Sys_GetTimeOfDay,
    Sys_CreateSemaphore,
    Sys_P,
    Sys_V,
    Sys_DestroySemaphore,
    Sys_PrintProcessList,
    Sys_PrintSysInfo,
    Sys_SelectPagingAlgorithm,
    /* File I/O system calls. */
    Sys_Mount,
    Sys_Open,
    Sys_OpenDirectory,
    Sys_Close,
    Sys_Delete,
    Sys_Read,
    Sys_ReadEntry,
    Sys_Write,
    Sys_Stat,
    Sys_FStat,
    Sys_Seek,
    Sys_CreateDir,
    Sys_Sync,
    Sys_Format,
    /* Pipe system calls. */
    Sys_CreatePipe,
    /* Message Queue system calls. */
    Sys_MessageQueueCreate,
    Sys_MessageQueueDestroy,
    Sys_MessageQueueSend,
    Sys_MessageQueueReceive,
    /* Memory Management system calls. */
    Sys_SBrk,
    /* Access Control */
    Sys_SetAcl,
    Sys_SetSetUid,
    Sys_SetEffectiveUid,
    Sys_GetUid,
};

/*
 * Number of system calls implemented.
 */
const int g_numSyscalls = sizeof(g_syscallTable) / sizeof(Syscall);
