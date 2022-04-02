/*
 * Paging (virtual memory) support
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.55 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/string.h>
#include <geekos/int.h>
#include <geekos/idt.h>
#include <geekos/irq.h>
#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/user.h>
#include <geekos/vfs.h>
#include <geekos/crc32.h>
#include <geekos/paging.h>
#include <geekos/keyboard.h>
#include <geekos/errno.h>
#include <geekos/timer.h>

#ifdef DEBUG
#ifndef PAGING_DEBUG
#define PAGING_DEBUG
#endif
#endif

/* ----------------------------------------------------------------------
 * Public data
 * ---------------------------------------------------------------------- */

pde_t* g_dir;		/* global pointer for kernel page directory */

int g_currentPagingAlgorithm = PAGING_DEFAULT; /* currently used paging algorithm */
ulong_t g_WorkingSet_Timeout = WORKING_SET_TIMEOUT; /* maximum page age in working set */

/* Statistic counters */
ulong_t g_PageFaults = 0;
ulong_t g_PageFileWrites = 0;
ulong_t g_PageFileReads = 0;
extern ulong_t g_PageAllocated;
extern ulong_t g_PageFreed;

/* ----------------------------------------------------------------------
 * Private functions/data
 * ---------------------------------------------------------------------- */

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)

#define PAGING_IRQ	14

static unsigned char* s_PageFileAdm;

/*
 * flag to indicate if debugging paging code
 */
#ifdef PAGING_DEBUG
int debugFaults = 1;
#else
int debugFaults = 0;
#endif
#define Debug(args...) if (debugFaults) Print(args)


void checkPaging()
{
  unsigned long reg=0;
  __asm__ __volatile__( "movl %%cr0, %0" : "=a" (reg));
  Print("Paging on ? : %d\n", (reg & (1<<31)) != 0);
}

/*
 * dump physical memory
 */
void Hex_Dump (void* paddr, ulong_t len) {
    int k, x, c;
    bool s2 = false;
    for (k=0; k < len; k+=16) {
        bool all_00 = true;
        for (x=0; x < 16; x++) if (*(uchar_t*)(paddr+k+x) != 0x00) all_00 = false;
        bool all_55 = true;
        for (x=0; x < 16; x++) if (*(uchar_t*)(paddr+k+x) != 0x55) all_55 = false;
        
        if (all_00 || all_55) { s2 = true; continue; }
        if (s2)  Print ("  skip ... %s\n", all_00 ? "zeros":"0x55");
        s2 = false;

        Print("%08lx  ", (ulong_t)paddr + k);
        for (x=0; x < 16; x++) {
            if (k+x < len)
                Print("%02x ", *(uchar_t*)(paddr+k+x));
            else
                Print("   ");
            if (x == 7)  Print("  ");
        }
        Print("  ");
        for (x=0; x < 16; x++) {
            c = *(uchar_t*)(paddr+k+x);
            if (k+x < len) Print("%c", c < 32 || c > 127 ? '.' : c);
            if (x == 7)  Print(" ");
        }
        Print("\n");
    }
}

/*
 * dump paging information
 */
void Dump_Paging_Info(void) {

    extern int unsigned s_numPages;
    extern uint_t g_freePageCount;

    struct Paging_Device *pageDev = Get_Paging_Device ();
    ulong_t pf = 0;
    int i, mx = pageDev->numSectors / SECTORS_PER_PAGE;
    char *a = "";

    for (i=0; i < mx; i++)
        if (s_PageFileAdm[i]) pf++;
    switch (g_currentPagingAlgorithm) {
        case PAGING_WSCLOCK: a = "WS-clock"; break;
        default: a = "default";
    }

    Print ("Paging algorithm %s\n", a);
    Print ("RAM Pages: total=%d, free=%d, faults=%ld, allocated=%ld, freed=%ld\n"
           "Page file: size=%d, writes=%ld, reads=%ld, paged out=%ld\n",
        s_numPages, g_freePageCount, g_PageFaults, g_PageAllocated, g_PageFreed,
        mx, g_PageFileWrites, g_PageFileReads, pf);



}

/*
 * Print diagnostic information for a page fault.
 */
static void Print_Fault_Info(uint_t address, faultcode_t faultCode)
{
    extern uint_t g_freePageCount;

    Print("Pid %d, Page Fault received, at address %x (%d pages free)\n",
        g_currentThread->pid, address, g_freePageCount);
    if (faultCode.protectionViolation)
        Print ("   Protection Violation, ");
    else
        Print ("   Non-present page, ");
    if (faultCode.writeFault)
        Print ("Write Fault, ");
    else
        Print ("Read Fault, ");
    if (faultCode.userModeFault)
        Print ("in User Mode\n");
    else
        Print ("in Supervisor Mode\n");
}

/*
 * Handler for page faults.
 * You should call the Install_Interrupt_Handler() function to
 * register this function as the handler for interrupt 14.
 */
static void Page_Fault_Handler(struct Interrupt_State* state)
{
    ulong_t address;
    faultcode_t faultCode;
    int pagefileIndex;

    KASSERT(!Interrupts_Enabled());

    /* Get the address that caused the page fault */
    address = Get_Page_Fault_Address();
    Debug("Page fault @%lx, pid=%d\n", address, g_currentThread->pid);

    /* Get the fault code */
    faultCode = *((faultcode_t *) &(state->errorCode));

    if ((address >= 0 && address < PAGE_SIZE) ||
        (address >= USER_BASE_ADDRESS && address < USER_BASE_ADDRESS+PAGE_SIZE) )
    {
        Print ("Null pointer dereferenced in pid %d.\n", g_currentThread->pid);
        Print_Fault_Info(address, faultCode);
        Dump_Interrupt_State(state);
        Exit(-1);
        KASSERT(0);
        return;
    }

    if (!faultCode.protectionViolation && faultCode.userModeFault) {
        ++g_PageFaults;
        pde_t * dir = g_currentThread->userContext->pageDir + PAGE_DIRECTORY_INDEX(address);
        pte_t * tab = 0;

        if (dir->present) {
            tab = (pte_t*)(dir->pageTableBaseAddr << PAGE_POWER) + PAGE_TABLE_INDEX(address);
        } else {
            int t;
            Debug("PF: allocating new page table at directory index %ld ##################\n",
                PAGE_DIRECTORY_INDEX(address) );
            tab = (pte_t*) Alloc_Resident_Page();
            KASSERT(tab != 0);

            dir->present = 1;
            dir->flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
            dir->accesed = 0;
            dir->reserved = 0;
            dir->largePages = 0;
            dir->globalPage = 0;
            dir->kernelInfo = 0;
            dir->pageTableBaseAddr = (ulong_t)tab >> PAGE_POWER;

            for (t=0; t < NUM_PAGE_TABLE_ENTRIES; t++) {
                tab[t].present = 0;
                tab[t].kernelInfo = 0;
            }
            tab += PAGE_TABLE_INDEX(address);
            Debug("PF: allocated new page table at 0x%08lx\n",
                (ulong_t)dir->pageTableBaseAddr << PAGE_POWER);
        }

        if (tab->kernelInfo == KINFO_PAGE_ON_DISK) {
            struct Page* page = 0;
            void* paddr = Alloc_Page();

            if (paddr != 0) {
                page = Get_Page((ulong_t) paddr);
                Debug ("PF: new page at addr %p (age = %ld)\n", paddr, page->clock);
//              KASSERT((page->flags & PAGE_PAGEABLE) == 0);
                /* Fill in accounting information for page */
                page->flags &= ~(PAGE_PAGEABLE);
                page->flags |= PAGE_LOCKED;
                page->entry = tab;
                page->entry->kernelInfo = 0;
                page->vaddr = PAGE_ADDR(address);
                KASSERT(page->flags & PAGE_ALLOCATED);
                Flush_TLB();

            } else {

                /* Select a page to steal from another process */
                Debug("PF: About to hunt for a page to page out\n");
                page = Find_Page_To_Page_Out();
                KASSERT(page->flags & PAGE_PAGEABLE);
                paddr = (void*) Get_Page_Address(page);
                Debug ("PF: Selected page at addr %p (age = %ld)\n", paddr, page->clock);

                /* Find a place on disk for it */
                pagefileIndex = Find_Space_On_Paging_File();
                if (pagefileIndex < 0) {
                    Print ("PF: Paging File is full. Aborting process pid=%d\n",
                        g_currentThread->pid);
                    /* we ran out of memory, so we just kill the thread/process. */
                    Exit(-1);
                    KASSERT(0);
                    return;
                }

                Debug("PF: Free disk page at index %d, for pid=%d\n",
                      pagefileIndex, g_currentThread->pid);

                /* Make the page temporarily unpageable (can't let another process steal it) */
                page->flags &= ~(PAGE_PAGEABLE);

                /* Lock the page so it cannot be freed while we're writing */
                page->flags |= PAGE_LOCKED;

                /* Write the page to disk. Interrupts are enabled, since the I/O may block. */
                Debug("PF: Writing physical frame %p (0x%08lx) to paging file at %d, flags=%x\n",
                    paddr, page->vaddr, pagefileIndex, page->entry->flags);
                Enable_Interrupts();
                Write_To_Paging_File(paddr, page->vaddr, pagefileIndex);
                Disable_Interrupts();

                /* While we were writing got notification this page isn't even needed anymore */
                if (page->flags & PAGE_ALLOCATED) {
                   /* The page is still in use update its bookeping info */
                   /* Update page table to reflect the page being on disk */
                   page->entry->present = 0;
                   page->entry->kernelInfo = KINFO_PAGE_ON_DISK;
                   page->entry->pageBaseAddr = pagefileIndex; /* Remember where it is located! */
                } else {
                   /* The page got freed, don't need bookeeping or it on disk */
                   Free_Space_On_Paging_File(pagefileIndex);

                   /* Its still allocated though to us now */
                   page->flags |= PAGE_ALLOCATED;
                }
            }
            KASSERT(!(page->flags & PAGE_PAGEABLE));

            pagefileIndex = tab->pageBaseAddr;

            /* Fill in accounting information for page */
            tab->present = 1;
//          tab->flags = VM_WRITE | VM_READ | VM_USER;
            tab->accesed = 0;
            tab->dirty = 0;
//          tab->pteAttribute = 0;
//          tab->globalPage = 0;
            tab->kernelInfo = 0;
            tab->pageBaseAddr = (ulong_t)paddr >> PAGE_POWER;
            page->entry = tab;
            page->vaddr = PAGE_ADDR(address);
            page->clock = (uint_t)g_numTicks;

            /* Read the page from disk. Interrupts are enabled, since the I/O may block. */
            Debug("PF: Reading physical frame %p (0x%08lx) from paging file at %d, flags=%x\n",
                paddr, page->vaddr, pagefileIndex, tab->flags);
            KASSERT(!(page->flags & PAGE_PAGEABLE));
            Enable_Interrupts();
            Read_From_Paging_File(paddr, page->vaddr, pagefileIndex);
            Disable_Interrupts();

            /* Free the page in paging file, since we read the content into memory */
            Debug ("PF: Freeing page in page file at index %d\n", pagefileIndex);
            Free_Space_On_Paging_File(pagefileIndex);

            /* Unlock the page */
            page->flags &= ~(PAGE_LOCKED);

            /* XXX - flush TLB should only flush the one page */
            Flush_TLB();

            page->flags |= PAGE_PAGEABLE;

            KASSERT(page->flags & PAGE_ALLOCATED);

        } else {
            if (!faultCode.writeFault)
                Print("PF: read uninitialized new page!\n");

            /* heap or stack is growing */
            tab->present = 1;
            tab->flags = VM_WRITE | VM_READ | VM_EXEC | VM_USER;
            tab->accesed = 0;
            tab->dirty = 0;
            tab->pteAttribute = 0;
            tab->globalPage = 0;
            tab->kernelInfo = 0;
            tab->pageBaseAddr = (ulong_t)
                Alloc_Pageable_Page (tab, PAGE_ADDR(address)) >> PAGE_POWER;
            KASSERT(tab->pageBaseAddr != 0);
            Debug ("\nAllocate additional page, uc=0x%08lx, pg=0x%08x va=0x%08lx\n",
                  (ulong_t)g_currentThread->userContext, tab->pageBaseAddr<<PAGE_POWER, address );
            Flush_TLB();
        }
        return;
    }

    /* rest of your handling code here */
    Print ("Unexpected Page Fault received\n");
    Print_Fault_Info(address, faultCode);
    Dump_Interrupt_State(state);
    /* user faults just kill the process */
    if (!faultCode.userModeFault) KASSERT(0);

    /* For now, just kill the thread/process. */
    Exit(-1);
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */


/*
 * Initialize virtual memory by building page tables
 * for the kernel and physical memory.
 */
void Init_VM(struct Boot_Info *bootInfo)
{
    /*
     * Hints:
     * - Build kernel page directory and page tables
     * - Call Enable_Paging() with the kernel page directory
     * - Install an interrupt handler for interrupt 14,
     *   page fault
     * - Do not map a page at address 0; this will help trap
     *   null pointer references
     */
    //TODO("Build initial kernel page directory and page tables");

    /* Build kernel page directory and page tables */
    int max_addr = bootInfo->memSizeKB/4 - 1;
    g_dir = (pde_t*) Alloc_Page();		/* kernel page directory */
    KASSERT(g_dir != 0);

    pte_t* tab;
    int d, t;

    for (d=0; d < NUM_PAGE_DIR_ENTRIES; d++) {
        if (d <= (max_addr>>10)) {
            Debug("Allocate table, didx=%d\n",d);
            tab = (pte_t*) Alloc_Page();	/* a kernel page table */
            KASSERT(tab != 0);
            g_dir[d].present = 1;
            g_dir[d].flags = VM_WRITE | VM_READ | VM_EXEC;
            g_dir[d].accesed = 0;
            g_dir[d].reserved = 0;
            g_dir[d].largePages = 0;
            g_dir[d].globalPage = 0;
            g_dir[d].kernelInfo = 0;
        //  g_dir[d].pageTableBaseAddr = PAGE_TABLE_INDEX((ulong_t)tab);
            g_dir[d].pageTableBaseAddr = (ulong_t)tab >> PAGE_POWER;

            for (t=0; t < NUM_PAGE_TABLE_ENTRIES; t++) {
               if ((d<<10)+t <= max_addr) {
                   tab[t].present = 1;
                   tab[t].flags = VM_WRITE | VM_READ | VM_EXEC;
                   tab[t].accesed = 0;
                   tab[t].dirty = 0;
                   tab[t].pteAttribute = 0;
                   tab[t].globalPage = 0;
                   tab[t].kernelInfo = 0;
                   tab[t].pageBaseAddr = (d<<10)+t;
               } else
                   tab[t].present = 0;
            }
            /* Do not map a page at address 0; this will help trap null
             * pointer references */
            if (!d) tab->present = 0;
        } else
            g_dir[d].present = 0;
    }

    /* Call Enable_Paging() with the kernel page directory */
    Enable_Paging (g_dir);

    /* Install an interrupt handler for interrupt 14, page fault */
    Install_Interrupt_Handler (PAGING_IRQ, Page_Fault_Handler);
}

/**
 * Initialize paging file data structures.
 * All filesystems should be mounted before this function
 * is called, to ensure that the paging file is available.
 */
void Init_Paging(void)
{
    // TODO("Initialize paging file data structures");

    struct Paging_Device *pageDev = Get_Paging_Device ();
    KASSERT (pageDev != 0);

    Debug ("Paging Device '%s', startSector=%ld, numSectors=%ld\n",
       pageDev->fileName, pageDev->startSector, pageDev->numSectors);

    s_PageFileAdm = (unsigned char*) Malloc (pageDev->numSectors / SECTORS_PER_PAGE);
    int i, mx = pageDev->numSectors / SECTORS_PER_PAGE;
    for (i=0; i < mx; i++)
        s_PageFileAdm[i] = 0;
}

/**
 * Find a free bit of disk on the paging file for this page.
 * Interrupts must be disabled.
 * @return index of free page sized chunk of disk space in
 *   the paging file, or -1 if the paging file is full
 */
int Find_Space_On_Paging_File(void)
{
    KASSERT(!Interrupts_Enabled());
    // TODO("Find free page in paging file");

    struct Paging_Device *pageDev = Get_Paging_Device ();
    KASSERT (pageDev != 0);

    int i, mx = pageDev->numSectors / SECTORS_PER_PAGE;
    for (i=0; i < mx; i++)
       if (!s_PageFileAdm[i]) {
           s_PageFileAdm[i] = 1;
           return i;
       }

    return EUNSPECIFIED;
}

/**
 * Free a page-sized chunk of disk space in the paging file.
 * Interrupts must be disabled.
 * @param pagefileIndex index of the chunk of disk space
 */
void Free_Space_On_Paging_File(int pagefileIndex)
{
    KASSERT(!Interrupts_Enabled());
    // TODO("Free page in paging file");

    struct Paging_Device *pageDev = Get_Paging_Device ();
    KASSERT (pageDev != 0);
    KASSERT (pagefileIndex >= 0 && pagefileIndex <= pageDev->numSectors / SECTORS_PER_PAGE);

    s_PageFileAdm[pagefileIndex] = 0;
}

/**
 * Write the contents of given page to the indicated block
 * of space in the paging file.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page is mapped in user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Write_To_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex)
{
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE)); /* Page must be locked! */
    // TODO("Write page data to paging file");
    ++g_PageFileWrites;

    struct Paging_Device *pageDev = Get_Paging_Device ();
    KASSERT (pageDev != 0);
    KASSERT (pagefileIndex >= 0 && pagefileIndex <= pageDev->numSectors / SECTORS_PER_PAGE);

    Debug ("Write_To_Paging_File:  paddr=%08lx, vaddr=%08lx, pfi=%d, sec=%ld\n",
        (ulong_t)paddr, vaddr,
        pagefileIndex, pageDev->startSector + SECTORS_PER_PAGE * pagefileIndex );

    int i;
    for (i=0; i < SECTORS_PER_PAGE; i++) {
       Block_Write (pageDev->dev, pageDev->startSector + SECTORS_PER_PAGE * pagefileIndex + i,
                    paddr + i*SECTOR_SIZE);
//     Hex_Dump (paddr + i*SECTOR_SIZE, SECTOR_SIZE);
    }
}

/**
 * Read the contents of the indicated block
 * of space in the paging file into the given page.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page will be re-mapped in
 *   user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Read_From_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex)
{
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE)); /* Page must be locked! */
    //TODO("Read page data from paging file");
    ++g_PageFileReads;

    struct Paging_Device *pageDev = Get_Paging_Device ();
    KASSERT (pageDev != 0);
    KASSERT (pagefileIndex >= 0 && pagefileIndex <= pageDev->numSectors / SECTORS_PER_PAGE);

    Debug ("Read_From_Paging_File: paddr=%08lx, vaddr=%08lx, pfi=%d, sec=%ld\n",
        (ulong_t)paddr, vaddr,
        pagefileIndex, pageDev->startSector + SECTORS_PER_PAGE * pagefileIndex );

    int i;
    for (i=0; i < SECTORS_PER_PAGE; i++) {
       Block_Read (pageDev->dev, pageDev->startSector + SECTORS_PER_PAGE * pagefileIndex + i,
                   paddr + i*SECTOR_SIZE);
//     Hex_Dump (paddr + i*SECTOR_SIZE, SECTOR_SIZE);
    }
}


/**
 * Reset the accessed and dirty bits of the physical availabe pages
 */
void Page_Cleaner(void)
{
#if PAGE_CLEARING_CYCLE > 1
    if (g_numTicks % PAGE_CLEARING_CYCLE != 0)  return;
#endif
//  Print("Page_Cleaner().\n");

    KASSERT(!Interrupts_Enabled());

    extern struct Page* g_pageList;
    extern int unsigned s_numPages;

    struct Page* p = g_pageList;
    struct Page* mx = g_pageList + s_numPages;
    pte_t* e;

    while (p < mx) {
       e = p->entry;
       if (e) {
          if (e->accesed) p->clock = g_numTicks;
          e->accesed = 0;
//        e->dirty = 0;
       }
       p++;
    }

}


/**
 * select the paging algorithm
 */
int SelectPagingAlgorithm(int alg) {

    switch (alg) {
        case PAGING_DEFAULT:
        case PAGING_WSCLOCK:
            g_currentPagingAlgorithm = alg;
            return 0;

        default:
            break;
    }

    return EUNSUPPORTED;
}

