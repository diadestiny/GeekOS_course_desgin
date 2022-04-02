/*
 * GeekOS file system
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.19 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#ifndef GEEKOS_GOSFS_H
#define GEEKOS_GOSFS_H

#include <geekos/blockdev.h>
#include <geekos/fileio.h>
#include <geekos/vfs.h>
#include <geekos/synch.h>


/* Magic */
#define GOSFS_MAGIC                 0x0DEADB05

#define GOSFS_NUM_INODES            1024

/* Number of disk sectors per filesystem block. */
#define GOSFS_SECTORS_PER_FS_BLOCK  8

/* Size of a filesystem block in bytes. */
#define GOSFS_FS_BLOCK_SIZE        (GOSFS_SECTORS_PER_FS_BLOCK*SECTOR_SIZE)

/* Flags bits for directory entries. */
#define GOSFS_INODE_USED            0x01    /* Directory entry is in use. */
#define GOSFS_INODE_ISDIRECTORY     0x02    /* Directory entry refers to a subdirectory. */
#define GOSFS_INODE_SETUID          0x04    /* File executes using uid of file owner. */

#define GOSFS_THIS_DIRECTORY        "."
#define GOSFS_PARENT_DIRECTORY      ".."

#define GOSFS_FILENAME_MAX          127     /* Maximum filename length. */

#define GOSFS_NUM_DIRECT_BLOCKS         8   /* Number of direct blocks in dir entry. */
#define GOSFS_NUM_INDIRECT_BLOCKS       1   /* Number of singly-indirect blocks in dir entry. */
#define GOSFS_NUM_2X_INDIRECT_BLOCKS    1   /* Number of doubly-indirect blocks in dir entry. */

/* Number of block pointers that can be stored in a single filesystem block. */
#define GOSFS_NUM_PTRS_PER_BLOCK     (GOSFS_FS_BLOCK_SIZE / sizeof(ulong_t))

/* Total number of block pointers in a directory entry. */
#define GOSFS_NUM_BLOCK_PTRS \
    (GOSFS_NUM_DIRECT_BLOCKS + GOSFS_NUM_INDIRECT_BLOCKS + GOSFS_NUM_2X_INDIRECT_BLOCKS)
    
#define GOSFS_NUM_INDIRECT_PTR_PER_BLOCK    (GOSFS_FS_BLOCK_SIZE / sizeof(ulong_t))
    
#define GOSFS_DIRTYP_THIS       1
#define GOSFS_DIRTYP_PARENT     2
#define GOSFS_DIRTYP_REGULAR    0
#define GOSFS_DIRTYP_FREE       -1

/*
#define GOSFS_AUTOSYNC	1	// 0: sync needed to write data to disc (superblock and file data)
							// 1: file data is written to disc immediately, sync needed for superblock
extern uint_t g_gosfs_sync;
*/

/*
 * A directory entry.
 * It is strongly recommended that you don't change this struct.
 */
/* we don't change it, but we don't use it
struct GOSFS_Dir_Entry {
    ulong_t size;           // Size of file. 
    ulong_t flags;          // Flags: used, isdirectory, setuid. 
    char filename[GOSFS_FILENAME_MAX+1];    // Filename (including space for nul-terminator). 
    ulong_t blockList[GOSFS_NUM_BLOCK_PTRS];    // Pointers to direct, indirect, and doubly-indirect blocks. 
    struct VFS_ACL_Entry acl[VFS_MAX_ACL_ENTRIES];// List of ACL entries; first is for the file's owner. 
};
*/


struct GOSFS_Inode {
    ulong_t inode;          // inode number
    ulong_t size;           // size of file in byte for files / or number of dir-entries for directories
    ulong_t link_count;     // link counter
    ulong_t blocks_used;    // number of blocks used by file
    ulong_t flags;          /* Flags: used, isdirectory, setuid. */
    ulong_t time_access;    // last access to file
    ulong_t time_modified;  // last modified
    ulong_t time_inode;     // last time inode changed
    ulong_t blockList[GOSFS_NUM_BLOCK_PTRS];    /* Pointers to direct, indirect, and doubly-indirect blocks. */
    struct VFS_ACL_Entry acl[VFS_MAX_ACL_ENTRIES];/* List of ACL entries; first is for the file's owner. */    
};

struct GOSFS_Directory 
{
    ulong_t type;           // type of entry
    ulong_t inode;          // referenced inode number
    char filename[GOSFS_FILENAME_MAX+1];    // filename
};

struct GOSFS_Superblock {
    ulong_t magic;
    //ulong_t p_root_dir;
    ulong_t supersize;      /* size of superblock in bytes */
    ulong_t size;           /* number of blocks of whole fs*/
    struct GOSFS_Inode inodes[GOSFS_NUM_INODES]; /* array of inodes of this fs*/
    uchar_t bitSet[0];      /* used/unused blocks */
};

/* on mount we create a GOSFS_Instance to work on */
struct GOSFS_Instance {
    struct Mutex lock;                    /* mutext to lock whole fs */
    struct FS_Buffer_Cache* buffercache;  /* buffer cache to work on */
    struct GOSFS_Superblock superblock;   /* superblock must be at the end of struct */
};

/* arbitrary data for file-descriptor */
struct GOSFS_FileEntry {
    struct GOSFS_Inode* inode;
    struct GOSFS_Instance* instance;
    uint_t references;                    /* the number of filedescriptors that references this entry  */
};

/* Number of directory entries that fit in a filesystem block. */
#define GOSFS_DIR_ENTRIES_PER_BLOCK    (GOSFS_FS_BLOCK_SIZE / sizeof(struct GOSFS_Directory))

void Init_GOSFS(void);

#endif
