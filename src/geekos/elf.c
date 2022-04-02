/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/fileio.h>
#include <geekos/elf.h>

#ifdef DEBUG
#ifndef ELF_DEBUG
#define ELF_DEBUG
#endif
#endif

/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat)
{
#ifdef ELF_DEBUG
    Print("\n\nAbout to parse ELF Header\n");
#endif
		
    elfHeader *exeHeader = (elfHeader*) exeFileData;
    programHeader *progHeader;
    unsigned char elfMagic[] = {0x7F, 'E', 'L', 'F'};    // Magic sould look like that
    unsigned int programHeaderOffset, programHeaderEntrySize, programHeaderNumEntries;
    int i;

    // Checking ELF-Magic
    //KASSERT(strncmp((char*) elfMagic, (char*) exeHeader->ident, 4));
    if (strncmp((char*) elfMagic, (char*) exeHeader->ident, 4)) {
#ifdef ELF_DEBUG
        Print("ELF-Magic number is wrong: %02x%02x%02x%02x %c%c%c%c\n",
           exeHeader->ident[0], exeHeader->ident[1], exeHeader->ident[2], exeHeader->ident[3],
           exeHeader->ident[0], exeHeader->ident[1], exeHeader->ident[2], exeHeader->ident[3]
        );
#endif
        return ENOEXEC;
    }

#ifdef ELF_DEBUG
    Print("Elf file detected: magic OK\n");
    Print("File is of type %d\n", exeHeader->type);
#endif

    // ony executables currently supported
    if (exeHeader->type != ET_EXEC) TODO("currently only Elf-executables are supported\n");
    if (exeHeader->machine != EM_386) TODO("currently only Intel architecture supported\n");

    exeFormat->entryAddr = exeHeader->entry;
    programHeaderOffset = exeHeader->phoff;
    programHeaderEntrySize = exeHeader->phentsize;
    programHeaderNumEntries = exeHeader->phnum;
    exeFormat->numSegments = programHeaderNumEntries;
#ifdef ELF_DEBUG
    Print("type: %d\n", exeHeader->type);
    Print("machine: %d\n", exeHeader->machine);
    Print("version: %d\n", exeHeader->version);
    Print("Entry adress is: %x\n", (unsigned int) exeFormat->entryAddr);
    Print("Program haeder offset is: %d\n", programHeaderOffset);
    Print("Program haeder entry size is: %d\n", programHeaderEntrySize);
    Print("Number of program header entries is: %d\n", programHeaderNumEntries);
#endif


    // loop over program heads to fetch segment information
    for (i=0; i < programHeaderNumEntries; i++)
    {
        // read program header
        progHeader = (programHeader*) (exeFileData + programHeaderOffset +
                                       (i*programHeaderEntrySize));
        
        exeFormat->segmentList[i].offsetInFile = progHeader->offset;
        exeFormat->segmentList[i].lengthInFile = progHeader->fileSize;
        exeFormat->segmentList[i].startAddress = progHeader->vaddr;
        exeFormat->segmentList[i].sizeInMemory = progHeader->memSize;
        exeFormat->segmentList[i].protFlags    = progHeader->flags;    
#ifdef ELF_DEBUG
        Print("processing Segment %d\n",i);
        Print("offsetInFile: %ld\n",exeFormat->segmentList[i].offsetInFile);
        Print("lengthInFile: %ld\n",exeFormat->segmentList[i].lengthInFile);
        Print("startAddress: %ld\n",exeFormat->segmentList[i].startAddress);
        Print("sizeInMemory: %ld\n",exeFormat->segmentList[i].sizeInMemory);
        Print("protFlags: %d\n",    exeFormat->segmentList[i].protFlags);
#endif
    }

#ifdef ELF_DEBUG
    Print("ELF Headers parsed\n");
#endif

    return 0;
}
