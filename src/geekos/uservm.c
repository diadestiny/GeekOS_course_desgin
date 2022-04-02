/*
 * Paging-based user mode implementation
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.50 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/paging.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/argblock.h>
#include <geekos/kthread.h>
#include <geekos/range.h>
#include <geekos/vfs.h>
#include <geekos/user.h>
#include <geekos/gdt.h>
#include <geekos/errno.h>

#ifdef DEBUG
#ifndef USERVM_DEBUG
#define USERVM_DEBUG
#endif
#endif

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

#ifdef USERVM_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif


void Print_Page_Directory (pde_t* dir) {

    Print("Print_Page_Directory()\n");

    pte_t* tab;

    int d, t;
    for (d=0; d < NUM_PAGE_DIR_ENTRIES; d++) {
        if (dir[d].present) {
            tab = (void*)(dir[d].pageTableBaseAddr << PAGE_POWER);
            Print("d=%04d     f=%x a=%x      lp=%x gp=%x inf=%x addr=%08lx\n",
                  d, dir[d].flags, dir[d].accesed, dir[d].largePages,
                  dir[d].globalPage, dir[d].kernelInfo, (ulong_t)tab);

            if (!tab)  continue;

            for (t=0; t < NUM_PAGE_TABLE_ENTRIES; t++) {
                if (tab[t].present) {
                    Print("    t=%4d f=%x a=%x d=%x  at=%x gp=%x inf=%x addr=%08lx\n",
                          t, tab[t].flags, tab[t].accesed, tab[t].dirty,
                          tab[t].pteAttribute, tab[t].globalPage, tab[t].kernelInfo,
                          (ulong_t)(tab[t].pageBaseAddr << PAGE_POWER) );
                }
            }
        }
    }
}



/*
 * Create a new user context of given size
 */

static struct User_Context* Create_User_Context(ulong_t size) {

    struct User_Context* uc;
    ulong_t sz = Round_Up_To_Page (size);

    Disable_Interrupts();
    uc = Malloc (sizeof (struct User_Context));

    if (!uc) {
        Enable_Interrupts();
        return 0;
    }

    /* Selector for the LDT's descriptor in the GDT */
    uc->ldtDescriptor = Allocate_Segment_Descriptor();;
    if (!uc->ldtDescriptor) {
        Enable_Interrupts();
        Debug ("No more segment descriptors available\n");
        return 0;
    }

    /* Build page directory and page tables */
    int min_addr = (USER_BASE_ADDRESS >> PAGE_POWER) + 1;
    int max_addr = min_addr + (Round_Up_To_Page(sz) >> PAGE_POWER) - 1;
    pde_t* dir = (pde_t*) Alloc_Page();         /* page directory */

    pte_t* tab;

    int d, t;
    for (d=0; d < NUM_PAGE_DIR_ENTRIES; d++) {
        if (g_dir[d].present) {
            Debug("Copy kernel table, didx=%d\n",d);
            dir[d] = g_dir[d];
        } else

        if (d >= (min_addr>>10) && d <= (max_addr>>10)) {
//          Debug("Allocate table, didx=%d\n",d);
            tab = (pte_t*) Alloc_Page();        /* page table */
            KASSERT(tab != 0);
// check tab != 0 without assertion, free allocated pages and return 0.
            dir[d].present = 1;
            dir[d].flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
            dir[d].accesed = 0;
            dir[d].reserved = 0;
            dir[d].largePages = 0;
            dir[d].globalPage = 0;
            dir[d].kernelInfo = 0;
            dir[d].pageTableBaseAddr = (ulong_t)tab >> PAGE_POWER;

            for (t=0; t < NUM_PAGE_TABLE_ENTRIES; t++) {
               if ((d<<10)+t >= min_addr && (d<<10)+t <= max_addr) {
//                 Debug("Allocate page, didx=%d, tbix=%d\n",d,t);
                   tab[t].present = 1;
                   tab[t].flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
                   tab[t].accesed = 0;
                   tab[t].dirty = 0;
                   tab[t].pteAttribute = 0;
                   tab[t].globalPage = 0;
                   tab[t].kernelInfo = 0;
                   tab[t].pageBaseAddr = (ulong_t)
                       Alloc_Pageable_Page (tab+t, ((d<<10)+t)<<PAGE_POWER) >> PAGE_POWER;
                   KASSERT(tab[t].pageBaseAddr != 0);
// check page address != 0 without assertion, free allocated pages and return 0.
                   Debug("Allocate page, uc=0x%08lx, didx=%4d, tbix=%4d, pg=0x%08x va=0x%08x\n",
                       (ulong_t)uc, d, t, tab[t].pageBaseAddr<<PAGE_POWER,
                       ((d<<10)+t)<<PAGE_POWER );
               } else {
                   tab[t].present = 0;
                   tab[t].kernelInfo = 0;
                   tab[t].flags = 0;
               }
            }
        } else {
            dir[d].present = 0;
            dir[d].flags = 0;
        }
    }


    /* allocate space for one page of stack memory at the end of the virtual address range */
    Debug("Allocate stack.\n");
    dir[NUM_PAGE_DIR_ENTRIES-1].present = 1;
    dir[NUM_PAGE_DIR_ENTRIES-1].flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
    tab = (pte_t*) Alloc_Page();        /* page table */
    KASSERT(tab != 0);
// check tab != 0 without assertion, free allocated pages and return 0.
    dir[NUM_PAGE_DIR_ENTRIES-1].pageTableBaseAddr = (ulong_t)tab >> PAGE_POWER;
    for (t=0; t < NUM_PAGE_TABLE_ENTRIES-1; t++) {
        tab[t].present = 0;
        tab[t].flags = 0;
        tab[t].accesed = 0;
        tab[t].dirty = 0;
        tab[t].pteAttribute = 0;
        tab[t].globalPage = 0;
        tab[t].kernelInfo = 0;
        tab[t].pageBaseAddr = 0;
    }

    t = NUM_PAGE_TABLE_ENTRIES-1;
    void* stack_page = Alloc_Pageable_Page (tab+t, USER_STACK_PAGE_ADDR);
    KASSERT(stack_page != 0);
// check stack page != 0 without assertion, free allocated pages and return 0.
    Debug("Allocated stack page = 0x%08lx\n", (ulong_t) stack_page);
    tab[t].present = 1;
    tab[t].flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
    tab[t].accesed = 0;
    tab[t].dirty = 0;
    tab[t].pteAttribute = 0;
    tab[t].globalPage = 0;
    tab[t].kernelInfo = 0;
    tab[t].pageBaseAddr = (ulong_t) stack_page >> PAGE_POWER;
    Debug("Allocate page, uc=0x%08lx, didx=%4d, tbix=%4d, pg=0x%08lx va=0x%08x\n",
        (ulong_t)uc, NUM_PAGE_DIR_ENTRIES-1, t, (ulong_t)stack_page, USER_STACK_PAGE_ADDR);

    uc->pageDir = dir;

    Enable_Interrupts();

    uc->memory = (void*)USER_BASE_ADDRESS;
    uc->size = USER_SEGMENT_LENGTH;

    /* initialize the local descriptor tables for code and data segmens */
    Init_LDT_Descriptor(uc->ldtDescriptor, uc->ldt, NUM_USER_LDT_ENTRIES);
    uc->ldtSelector = Selector(KERNEL_PRIVILEGE, true, Get_Descriptor_Index(uc->ldtDescriptor));
    Init_Code_Segment_Descriptor (&uc->ldt[0], (ulong_t) uc->memory, uc->size/PAGE_SIZE,
                                  USER_PRIVILEGE);
    Init_Data_Segment_Descriptor (&uc->ldt[1], (ulong_t) uc->memory, uc->size/PAGE_SIZE,
                                  USER_PRIVILEGE);
    uc->csSelector = Selector (USER_PRIVILEGE, false, 0);
    uc->dsSelector = Selector (USER_PRIVILEGE, false, 1);

    uc->entryAddr = 0;
    uc->argBlockAddr = 0;
    uc->stackPointerAddr = 0;
    uc->refCount = 0;
    uc->eUId = 0;

#ifdef USERVM_DEBUG
    Print_Page_Directory (dir);
#endif

    memset (uc->iob, '\0', sizeof(struct File*) * USER_MAX_FILES);
//  for (d=0; d < USER_MAX_FILES; d++)  uc->iob[d] = 0;

    return uc;
}


/*
 * Copy data from kernel buffer into user buffer.
 * Returns true if successful, false otherwise.
 */
bool __copy_to_user(struct User_Context* context, ulong_t destInUser,
                    void* srcInKernel, ulong_t numBytes)
{
    /*
     * Hints:
     * - Make sure that user page is part of a valid region
     *   of memory
     * - Remember that you need to add 0x80000000 to user addresses
     *   to convert them to kernel addresses, because of how the
     *   user code and data segments are defined
     * - User pages may need to be paged in from disk before being accessed.
     * - Before you touch (read or write) any data in a user
     *   page, **disable the PAGE_PAGEABLE bit**.
     *
     * Be very careful with race conditions in reading a page from disk.
     * Kernel code must always assume that if the struct Page for
     * a page of memory has the PAGE_PAGEABLE bit set,
     * IT CAN BE STOLEN AT ANY TIME.  The only exception is if
     * interrupts are disabled; because no other process can run,
     * the page is guaranteed not to be stolen.
     */
    KASSERT(context != 0);
    KASSERT(context->pageDir != 0);

    pde_t* dir = context->pageDir;
    pte_t* tab;
    struct Page* page;
    bool iflag;

    Debug("__copy_to_user: context=0x%08lx  dir=0x%08lx\n", (ulong_t)context, (ulong_t)dir);

    ulong_t anf_p, phy_a, phy_e, end_p, cp_a, x, cp_l;
    ulong_t a = (ulong_t)context->memory + destInUser;
    ulong_t e = a + numBytes - 1;
    void*   s = srcInKernel;
    Debug("__copy_to_user addr=0x%08lx end=0x%08lx src=0x%08lx  len=%ld\n",
          a, e, (ulong_t)s, numBytes);

    while (a <= e) {
        anf_p = a & (~PAGE_MASK);

        iflag = Begin_Int_Atomic();
        if (!dir[PAGE_DIRECTORY_INDEX(anf_p)].present) {
            Print("__copy_to_user: page directory not present: p=0x%08lx dir=0x%08lx idx=%ld\n",
                anf_p, (ulong_t)dir, PAGE_DIRECTORY_INDEX(anf_p));
            End_Int_Atomic(iflag);
            return false;
        }

        tab = (pte_t*)(dir[PAGE_DIRECTORY_INDEX(anf_p)].pageTableBaseAddr << PAGE_POWER);

        if ( !tab[PAGE_TABLE_INDEX(anf_p)].present
           && tab[PAGE_TABLE_INDEX(anf_p)].kernelInfo != KINFO_PAGE_ON_DISK) {
            Print("__copy_to_user: page table not present: "
                  "p=0x%08lx dir=0x%08lx didx=%ld tab=0x%08lx tidx=%ld\n",
                anf_p, (ulong_t)dir, PAGE_DIRECTORY_INDEX(anf_p), (ulong_t)tab,
                PAGE_TABLE_INDEX(anf_p));
            End_Int_Atomic(iflag);
            return false;
        }

        if (tab[PAGE_TABLE_INDEX(anf_p)].present) {
            phy_a = tab[PAGE_TABLE_INDEX(anf_p)].pageBaseAddr << PAGE_POWER;
            page = Get_Page (phy_a);
        } else {
            int pagefileIndex = tab[PAGE_TABLE_INDEX(anf_p)].pageBaseAddr;

            Debug("__copy_to_user: Read_From_Paging_File\n");
            phy_a = (ulong_t) Alloc_Pageable_Page (tab + PAGE_TABLE_INDEX(anf_p), (ulong_t)anf_p);

            page = Get_Page (phy_a);
            page->entry->pageBaseAddr = phy_a >> PAGE_POWER;
            page->flags &= ~(PAGE_PAGEABLE);
            page->flags |= PAGE_LOCKED;

            KASSERT(!(page->flags & PAGE_PAGEABLE));
            Enable_Interrupts();
            Read_From_Paging_File ((void*)phy_a, anf_p, pagefileIndex);
            Disable_Interrupts();
            Free_Space_On_Paging_File(pagefileIndex);
            page->entry->present = 1;
            page->entry->kernelInfo = 0;
            /* XXX - flush TLB should only flush the one page */
            Flush_TLB();
        }

        page->flags &= ~(PAGE_PAGEABLE);
        page->flags |= PAGE_LOCKED;

        KASSERT(page->flags & PAGE_ALLOCATED);

        end_p = anf_p | PAGE_MASK;

        cp_a = (a & PAGE_MASK) + phy_a;
        x = (end_p > e) ? e : end_p;
        phy_e = (x & PAGE_MASK) + phy_a;
        cp_l = phy_e - cp_a + 1;

        Debug("memcpy dst=0x%08lx (0x%08lx)  src=0x%08lx  len=%ld\n",
            a, cp_a, (ulong_t)s, cp_l);

        memcpy ((void*)cp_a, s, cp_l);

        page->flags |= PAGE_PAGEABLE;
        page->flags &= ~(PAGE_LOCKED);

        End_Int_Atomic(iflag);

        s += PAGE_SIZE - (a - anf_p);
        a += PAGE_SIZE - (a - anf_p);
        if (!a)  break;
    }

    return true;
}


/*
 * Copy data from user buffer into kernel buffer.
 * Returns true if successful, false otherwise.
 */
bool __copy_from_user(struct User_Context* context, void* destInKernel,
                      ulong_t srcInUser, ulong_t numBytes)
{
    /*
     * Hints:
     * - Make sure that user page is part of a valid region
     *   of memory
     * - Remember that you need to add 0x80000000 to user addresses
     *   to convert them to kernel addresses, because of how the
     *   user code and data segments are defined
     * - User pages may need to be paged in from disk before being accessed.
     * - Before you touch (read or write) any data in a user
     *   page, **disable the PAGE_PAGEABLE bit**.
     *
     * Be very careful with race conditions in reading a page from disk.
     * Kernel code must always assume that if the struct Page for
     * a page of memory has the PAGE_PAGEABLE bit set,
     * IT CAN BE STOLEN AT ANY TIME.  The only exception is if
     * interrupts are disabled; because no other process can run,
     * the page is guaranteed not to be stolen.
     */
    KASSERT(context != 0);
    KASSERT(context->pageDir != 0);

    pde_t* dir = context->pageDir;
    pte_t* tab;
    struct Page* page;
    bool iflag;

    Debug("__copy_from_user: context=0x%08lx  dir=0x%08lx\n", (ulong_t)context, (ulong_t)dir);

    ulong_t anf_p, phy_a, phy_e, end_p, cp_a, x, cp_l;
    ulong_t a = (ulong_t)context->memory + srcInUser;
    ulong_t e = a + numBytes - 1;
    void*   dst = destInKernel;
    Debug("__copy_from_user addr=0x%08lx end=0x%08lx dst=0x%08lx  len=%ld\n",
          a, e, (ulong_t)dst, numBytes);

    while (a <= e) {
        anf_p = a & ~(PAGE_MASK);

        iflag = Begin_Int_Atomic();
        if (!dir[PAGE_DIRECTORY_INDEX(anf_p)].present) {
            Debug("__copy_from_user: addr=0x%08lx dst=0x%08lx  len=%ld\n",
                (ulong_t)context->memory + srcInUser, (ulong_t)dst, numBytes);
            Print("__copy_from_user: page directory entry not present: "
                  "p=0x%08lx dir=0x%08lx idx=%ld\n",
                anf_p, (ulong_t)dir, PAGE_DIRECTORY_INDEX(anf_p));
            End_Int_Atomic(iflag);
            return false;
        }

        tab = (pte_t*)(dir[PAGE_DIRECTORY_INDEX(anf_p)].pageTableBaseAddr << PAGE_POWER);

        if ( !tab[PAGE_TABLE_INDEX(anf_p)].present
           && tab[PAGE_TABLE_INDEX(anf_p)].kernelInfo != KINFO_PAGE_ON_DISK) {
            Print("__copy_from_user: page table entry not present: "
                  "p=0x%08lx dir=0x%08lx didx=%ld tab=0x%08lx tidx=%ld\n",
                anf_p, (ulong_t)dir, PAGE_DIRECTORY_INDEX(anf_p), (ulong_t)tab,
                PAGE_TABLE_INDEX(anf_p));
            End_Int_Atomic(iflag);
            return false;
        }

        if (tab[PAGE_TABLE_INDEX(anf_p)].present) {
            phy_a = tab[PAGE_TABLE_INDEX(anf_p)].pageBaseAddr << PAGE_POWER;
            page = Get_Page (phy_a);
        } else {
            int pagefileIndex = tab[PAGE_TABLE_INDEX(anf_p)].pageBaseAddr;

            Debug("__copy_from_user: Read_From_Paging_File\n");
            phy_a = (ulong_t) Alloc_Pageable_Page (tab + PAGE_TABLE_INDEX(anf_p), (ulong_t)anf_p);
            page = Get_Page (phy_a);
            page->entry->pageBaseAddr = phy_a >> PAGE_POWER;
            page->flags &= ~(PAGE_PAGEABLE);
            page->flags |= PAGE_LOCKED;

            KASSERT(!(page->flags & PAGE_PAGEABLE));
            Enable_Interrupts();
            Read_From_Paging_File ((void*)phy_a, anf_p, pagefileIndex);
            Disable_Interrupts();
            Free_Space_On_Paging_File(pagefileIndex);
            page->entry->present = 1;
            page->entry->kernelInfo = 0;
            /* XXX - flush TLB should only flush the one page */
            Flush_TLB();
        }

        page->flags &= ~(PAGE_PAGEABLE);
        page->flags |= PAGE_LOCKED;

        KASSERT(page->flags & PAGE_ALLOCATED);

        end_p = anf_p | PAGE_MASK;

        cp_a = (a & PAGE_MASK) + phy_a;
        x = (end_p > e) ? e : end_p;
        phy_e = (x & PAGE_MASK) + phy_a;
        cp_l = phy_e - cp_a + 1;

        Debug("memcpy dst=0x%08lx (0x%08lx)  src=0x%08lx  len=%ld\n",
            a, cp_a, (ulong_t)dst, cp_l);

        memcpy (dst,(void*)cp_a, cp_l);

        page->flags |= PAGE_PAGEABLE;
        page->flags &= ~(PAGE_LOCKED);

        End_Int_Atomic(iflag);

        dst += PAGE_SIZE - (a - anf_p);
        a += PAGE_SIZE - (a - anf_p);
        if (!a)  break;
    }

    return true;
}



/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context* context)
{
    /*
     * Hints:
     * - Free all pages, page tables, and page directory for
     *   the process (interrupts must be disabled while you do this,
     *   otherwise those pages could be stolen by other processes)
     * - Free semaphores, files, and other resources used
     *   by the process
     */
    //TODO("Destroy User_Context data structure after process exits");

    bool iflag;
    Debug ("Destroy_User_Context(), context=%08x\n", (unsigned int)context);

    KASSERT (context->refCount == 0);
    KASSERT (context->pageDir != 0);

    Free_Segment_Descriptor (context->ldtDescriptor);

    iflag = Begin_Int_Atomic();

    Set_PDBR (g_dir);
    // Flush_TLB ();

    pde_t* dir = context->pageDir;
    pte_t* tab;
    
    int d, t;
    for (d=0; d < NUM_PAGE_DIR_ENTRIES; d++) {
#if 0
        Debug("Destroy_User_Context() dir=%08lx, d=%d, p=%d, usr=%d\n",
              (ulong_t)dir, d, dir[d].present, dir[d].flags & VM_USER ? 1:0);
#endif
        if (dir[d].present && dir[d].flags & VM_USER) {
#if 0
	   Print("Try 1 to free dir=%d\n", d);
           Print("Try 2 to free dir=%d, table=%08x\n",
                 d, dir[d].pageTableBaseAddr << PAGE_POWER);
#endif
           tab = (pte_t*) (dir[d].pageTableBaseAddr << PAGE_POWER);
           KASSERT(dir[d].pageTableBaseAddr != 0);

           for (t=0; t < NUM_PAGE_TABLE_ENTRIES; t++) {
               if (tab[t].present) {
#if 0
                   Print("Free table dir=%d, tab=%d, table=%08x\n",
                         d, t, tab[t].pageBaseAddr << PAGE_POWER);
#endif
                   Free_Page ((void*)(tab[t].pageBaseAddr << PAGE_POWER));
#if 0
                   Print("Free table dir=%d, tab=%d, table=%08x, freed\n",
                         d, t, tab[t].pageBaseAddr << PAGE_POWER);
#endif
               } else
               if (tab[t].kernelInfo == KINFO_PAGE_ON_DISK)
                  Free_Space_On_Paging_File (tab[t].pageBaseAddr);
           }
           Free_Page (tab);
        }
    }
    Free_Page (dir);

    //TODO: free semaphores, files, and other resources: -> not here! but in Exit()

    /* Close all open files */
    for (d=0; d < USER_MAX_FILES; d++) {
        if (context->iob[d]) {
            Debug ("Destroy_User_Context() close pid=%d fd=%d fda=%p\n",
                   g_currentThread->pid, d, context->iob[d]);
            Close (context->iob[d]);
        }
    }

    Free (context);

    Debug ("Destroy_User_Context() end.\n");
    End_Int_Atomic(iflag);
}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat, const char *command,
    struct User_Context **pUserContext)
{
    /*
     * Hints:
     * - This will be similar to the same function in userseg.c
     * - Determine space requirements for code, data, argument block,
     *   and stack
     * - Allocate pages for above, map them into user address
     *   space (allocating page directory and page tables as needed)
     * - Fill in initial stack pointer, argument block address,
     *   and code entry point fields in User_Context
     */
    //TODO("Load user program into address space");

    Debug ("Load_User_Program(), cmd='%s'\n", command);

    /* we determine size of argument block */
    unsigned argc;
    ulong_t argvSize;
    Get_Argument_Block_Size(command, &argc, &argvSize);

    /* we determine the maximum address in memory */
    int i;
    struct Exe_Segment *s;
    ulong_t maxAddr = 0;

    s = exeFormat->segmentList;
    for (i=0; i < exeFormat->numSegments; i++, s++) {
       ulong_t segMaxAddr = s->startAddress + s->sizeInMemory;
       if (segMaxAddr > maxAddr)  maxAddr = segMaxAddr;
    }

    /* we place the argument block on top of the memory (will be the stack later) */
    ulong_t size = Round_Up_To_Page(maxAddr);

    /* now we create the user context (hopefully) */
    struct User_Context *uc;
    uc = Create_User_Context (size + INITIAL_HEAP_SIZE);

    if (!uc)  return ENOMEM;

    Debug ("Load_User_Program() 1.\n");
    s = exeFormat->segmentList;
    for (i = 0; i < exeFormat->numSegments; i++, s++)
       Debug("Seg %d, m=0x%08lx st=0x%08lx ad=0x%08lx, ext=0x%08lx, l=%ld\n",
           i, (ulong_t)uc->memory, s->startAddress, (ulong_t)uc->memory + s->startAddress,
           (ulong_t)exeFileData + s->offsetInFile, s->lengthInFile);

    /* here we copy each executable segment into memory */
    s = exeFormat->segmentList;
    for (i = 0; i < exeFormat->numSegments; i++, s++)
        if (s->lengthInFile > 0)
           __copy_to_user (uc, s->startAddress, exeFileData + s->offsetInFile, s->lengthInFile);

    Debug ("Load_User_Program() 3.\n");
    /* now we format the argument block in memory */
    ulong_t argv = USER_STACK_ADDRESS - USER_BASE_ADDRESS - argvSize;
    void* argBlock = Malloc(argvSize);
    Debug ("Load_User_Program() 4. argv=0x%08lx sz=%ld\n", (ulong_t)argv, argvSize);
    Format_Argument_Block (argBlock, argc, argv, command);
    Debug ("Load_User_Program() 5.\n");
    __copy_to_user (uc, argv, argBlock, argvSize);
    Debug ("Load_User_Program() 6.\n");
    Free(argBlock);

    Debug ("Load_User_Program() 7.\n");
    /* In the created User_Context object, we set code entry point address,
     * argument block address, and initial kernel stack pointer address */
    uc->entryAddr = exeFormat->entryAddr;
    uc->argBlockAddr = argv;
    uc->stackPointerAddr = argv; /* arguments are passed via the stack to main() */
    uc->size = size + INITIAL_HEAP_SIZE;  /* maximum code address rounded up to next page address */
    *pUserContext = uc;

    Debug ("Load_User_Program() finished.\n");
    return 0;
}

/*
 * Copy data from user buffer into kernel buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_From_User(void* destInKernel, ulong_t srcInUser, ulong_t numBytes)
{
    /*
     * Hints:
     * - Make sure that user page is part of a valid region
     *   of memory
     * - Remember that you need to add 0x80000000 to user addresses
     *   to convert them to kernel addresses, because of how the
     *   user code and data segments are defined
     * - User pages may need to be paged in from disk before being accessed.
     * - Before you touch (read or write) any data in a user
     *   page, **disable the PAGE_PAGEABLE bit**.
     *
     * Be very careful with race conditions in reading a page from disk.
     * Kernel code must always assume that if the struct Page for
     * a page of memory has the PAGE_PAGEABLE bit set,
     * IT CAN BE STOLEN AT ANY TIME.  The only exception is if
     * interrupts are disabled; because no other process can run,
     * the page is guaranteed not to be stolen.
     */
    //TODO("Copy user data to kernel buffer");

    Debug("Copy_From_User: 0x%08lx <- 0x%08lx x 0x%08lx\n", (ulong_t)destInKernel,
        srcInUser, numBytes);

    struct User_Context* context = g_currentThread->userContext;

    KASSERT(context != 0);

    return __copy_from_user (context, destInKernel, srcInUser, numBytes);
}


/*
 * Copy data from kernel buffer into user buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_To_User(ulong_t destInUser, void* srcInKernel, ulong_t numBytes)
{
    /*
     * Hints:
     * - Same as for Copy_From_User()
     * - Also, make sure the memory is mapped into the user
     *   address space with write permission enabled
     */
    //TODO("Copy kernel data to user buffer");

    Debug("Copy_To_User: 0x%08lx <- 0x%08lx x 0x%08lx\n", destInUser,
        (ulong_t)srcInKernel, numBytes);

    struct User_Context* context = g_currentThread->userContext;

    KASSERT(context != 0);

    return __copy_to_user (context, destInUser, srcInKernel, numBytes);
}

/*
 * Switch to user address space.
 */
void Switch_To_Address_Space(struct User_Context *userContext)
{
    /*
     * - If you are still using an LDT to define your user code and data
     *   segments, switch to the process's LDT
     * - 
     */
    //TODO("Switch_To_Address_Space() using paging");

    Debug ("Switch_To_Address_Space(), userContext=%08lx, selector=%d\n",
           (ulong_t)userContext, userContext->ldtSelector);

    KASSERT(!Interrupts_Enabled());
    KASSERT(userContext->ldtSelector != 0);
    KASSERT(userContext->pageDir != 0);

    ushort_t s = userContext->ldtSelector;

    __asm__ __volatile__ ("lldt %0" : : "a" (s));

    Set_PDBR (userContext->pageDir);
    // Flush_TLB ();
}

