/*
 * GeekOS file system
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.54 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <limits.h>
#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/bitset.h>
#include <geekos/synch.h>
#include <geekos/int.h>
#include <geekos/bufcache.h>
#include <geekos/gosfs.h>
#include <geekos/user.h>
#include <libc/sched.h>

#ifdef DEBUG
#ifndef FS_DEBUG
#define FS_DEBUG
#endif
#endif

#ifdef FS_DEBUG
#define Debug(args...) Print(args)
#else
#define Debug(args...)
#endif

/* ----------------------------------------------------------------------
 * 关于 VFS虚拟文件系统 的接口
 * ---------------------------------------------------------------------- */

/* 查找下一个空闲索引节点inode */
int Find_Free_Inode(struct Mount_Point *mountPoint, ulong_t *retInode)
{
    int rc=-1;
    ulong_t i;
    
    for (i=0; i<GOSFS_NUM_INODES; i++)
    {
        if (((struct GOSFS_Instance*)(mountPoint->fsData))->superblock.inodes[i].flags == 0)
        {
            rc=0;
            *retInode=i;
            break;
        }
    }
    
    return rc;
}


/* 检查目录是否为空 */
bool IsDirectoryEmpty(struct GOSFS_Inode* pInode, struct GOSFS_Instance* p_instance)
{
    bool rc = 1, rc2 = 0;
    int i = 0, e = 0;
    struct FS_Buffer* p_buff = 0;
    ulong_t blockNum = 0;
    struct GOSFS_Directory*    tmpDir = 0;
    
    // 检查是否为一个目录 
    if (!(pInode->flags & GOSFS_INODE_ISDIRECTORY)) 
        goto finish;
    
    // 检查是否存在包含非空目录的BLOCK
    for (i=0; i<GOSFS_NUM_DIRECT_BLOCKS; i++)
    {
        blockNum = pInode->blockList[i];
        if (blockNum != 0)
        {
            Debug("found direct block %ld\n",blockNum);
            rc2 = Get_FS_Buffer (p_instance->buffercache, blockNum, &p_buff);
            for (e=0; e<GOSFS_DIR_ENTRIES_PER_BLOCK; e++)
            {
                //Debug("checking directory entry %d\n",e);
                tmpDir = (struct GOSFS_Directory*) ( (p_buff->data)+(e*sizeof(struct GOSFS_Directory)) );
                if (tmpDir->type == GOSFS_DIRTYP_REGULAR)
                {
                    Debug("found used directory %d(%s) in block %ld\n", e, tmpDir->filename, blockNum);
                    rc = 0;
                    goto finish;
                }
            }
            
            rc2 = Release_FS_Buffer(p_instance->buffercache, p_buff);
            p_buff = 0;
            
            if (rc2<0)
            {
                Debug("Unable to release fs_buffer for new-directory\n");
                rc = EFSGEN;
                goto finish;
            }
        }
    }


finish:
    if (p_buff != 0)  Release_FS_Buffer(p_instance->buffercache, p_buff);
    Debug("IsDirectoryEmpty returns %d\n", (int)rc);
    return rc;
}


/* 计算指定字节数所需的块数  */
int FindNumBlocks(ulong_t size)
{
    return (((size-1)/GOSFS_FS_BLOCK_SIZE)+1);
}


/* 将超级块从内存写入磁盘  */
int WriteSuperblock(struct GOSFS_Instance *p_instance)
{
    int numBlocks, rc=0;
    ulong_t numBytes, bwritten=0, i;
    struct FS_Buffer *p_buff=0;
    
    numBytes = p_instance->superblock.supersize;
    numBlocks = FindNumBlocks(numBytes);
    
    for (i=0; i<numBlocks; i++)
    {
        
        rc = Get_FS_Buffer(p_instance->buffercache,i,&p_buff);
        if ((p_instance->superblock.supersize - bwritten) < GOSFS_FS_BLOCK_SIZE)
        {
            memcpy(p_buff->data, ((void*)&(p_instance->superblock))+bwritten, p_instance->superblock.supersize - bwritten);
            bwritten = bwritten + (p_instance->superblock.supersize - bwritten);
        }
        else
        {
            memcpy(p_buff->data, ((void*)&(p_instance->superblock))+bwritten, GOSFS_FS_BLOCK_SIZE);
            bwritten = bwritten+GOSFS_FS_BLOCK_SIZE;
        }
        
        Modify_FS_Buffer(p_instance->buffercache,p_buff);
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
    }
        
finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    return rc;
}



/* 为inode创建第一个带有目录项的block */
/* including . and .. directories */
int CreateFirstDirectoryBlock(ulong_t thisInode, ulong_t parentInode, struct FS_Buffer *p_buff)
{
    struct GOSFS_Directory dirEntry;
    int i;
    
    // 为根目录创建 "." and ".."    
    for (i=0; i<GOSFS_DIR_ENTRIES_PER_BLOCK; i++)
    {
        if (i==0)    // this directory entry "."
        {
            dirEntry.type = GOSFS_DIRTYP_THIS;
            dirEntry.inode = thisInode;
            strcpy(dirEntry.filename, GOSFS_THIS_DIRECTORY);
        }
        else if (i==1)    // parent directory entry ".."
        {
            dirEntry.type = GOSFS_DIRTYP_PARENT;
            dirEntry.inode = parentInode;
            strcpy(dirEntry.filename, GOSFS_PARENT_DIRECTORY);
        }
        else
        {
            dirEntry.type = GOSFS_DIRTYP_FREE;
            dirEntry.inode = 0;
            strcpy(dirEntry.filename, "\0");
        }
        
        memcpy(p_buff->data + (i*sizeof(struct GOSFS_Directory)), &dirEntry, sizeof(struct GOSFS_Directory));
    }
    
    return 0;
}

/* 为inode创建下一个带有目录项的block */
int CreateNextDirectoryBlock(struct FS_Buffer *p_buff)
{
    struct GOSFS_Directory dirEntry;
    int i;
    
    for (i=0; i<GOSFS_DIR_ENTRIES_PER_BLOCK; i++)
    {
        dirEntry.type = GOSFS_DIRTYP_FREE;
        dirEntry.inode = 0;
        strcpy(dirEntry.filename, "\0");
        
        memcpy(p_buff->data + (i*sizeof(struct GOSFS_Directory)), &dirEntry, sizeof(struct GOSFS_Directory));
    }
    
    return 0;
}


/*通过inode删除目录项 */
int RemoveDirEntryFromInode(struct GOSFS_Instance *p_instance, ulong_t parentInode, ulong_t inode)
{
    int rc = 0, i = 0, e = 0;
    struct FS_Buffer *p_buff = 0;
    struct GOSFS_Directory* tmpDir = 0;
    ulong_t blockNum=0;
    
    Debug("About to remove inode %ld from dir-inode %ld\n",inode, parentInode);
    
    // 在目录项中搜索要删除的inode 
    for (i=0; i<GOSFS_NUM_DIRECT_BLOCKS; i++)
    {
        blockNum = p_instance->superblock.inodes[parentInode].blockList[i];
        if (blockNum != 0)
        {        
            rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
            
            for (e=0; e<GOSFS_DIR_ENTRIES_PER_BLOCK; e++)
            {
                tmpDir = (struct GOSFS_Directory*)((p_buff->data)+(e*sizeof(struct GOSFS_Directory)));
                if (tmpDir->inode == inode)
                {
                    Debug("found directory entry %d in Block %ld\n",e,blockNum);
                    tmpDir->inode=0;
                    tmpDir->type=GOSFS_DIRTYP_FREE;
                    strcpy(tmpDir->filename, "\0");
                    
                    Modify_FS_Buffer(p_instance->buffercache,p_buff);

                    // 减少parent_inode的数量
                    p_instance->superblock.inodes[parentInode].size--;

                    goto finish;
                }
            }

            rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
            p_buff = 0;

        }
    }
finish:
    if (p_buff != 0)  Release_FS_Buffer(p_instance->buffercache, p_buff);
    return rc;
}


/* 向inode添加目录项 */
int AddDirEntry2Inode(struct GOSFS_Instance *p_instance, ulong_t parentInode, struct GOSFS_Directory *dirEntry)
{
    int i=0,e=0,rc=0,found=0, numDirectPtr=-1;
    ulong_t blockNum;
    struct FS_Buffer *p_buff=0;
    struct GOSFS_Directory *tmpDir=0;
    
    //在 GOSFS_Directory里面寻找空闲块
    for (i=0; i<GOSFS_NUM_DIRECT_BLOCKS; i++)
    {
        if (found==1) break;
            
        blockNum = p_instance->superblock.inodes[parentInode].blockList[i];
        if (blockNum != 0)
        {
            Debug("found direct block %ld\n",blockNum);
            rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
            for (e=0; e < GOSFS_DIR_ENTRIES_PER_BLOCK; e++)
            {
                //Debug("checking directory entry %d\n",e);
                tmpDir = (struct GOSFS_Directory*)((p_buff->data)+(e*sizeof(struct GOSFS_Directory)));
                if (tmpDir->type == GOSFS_DIRTYP_FREE)
                {
                    Debug("found free directory %d in block %ld\n",e,blockNum);
                    
                    memcpy((p_buff->data)+(e*sizeof(struct GOSFS_Directory)), dirEntry, sizeof(struct GOSFS_Directory));
                    found=1;
                    Modify_FS_Buffer(p_instance->buffercache,p_buff);
                    p_instance->superblock.inodes[parentInode].size++;
                    break;
                }
            }
            
            rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
            p_buff = 0;
            
        }
    }
    
    //找不到空闲目录项,创建一个新块
    numDirectPtr=-1;
    if ((found==0) && (numDirectPtr>=0))
    {    
        blockNum=Find_First_Free_Bit(p_instance->superblock.bitSet, p_instance->superblock.size);
        Debug("found free directory 0 in block %ld\n",blockNum);
        rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
        rc = CreateNextDirectoryBlock(p_buff);
        // 从开始位置拷贝目录
        memcpy(p_buff->data, dirEntry, sizeof(struct GOSFS_Directory));
        Modify_FS_Buffer(p_instance->buffercache,p_buff);
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
        p_instance->superblock.inodes[parentInode].blockList[numDirectPtr]=blockNum;
        p_instance->superblock.inodes[parentInode].size++;
        Set_Bit(p_instance->superblock.bitSet, blockNum);
        found = 1;
        goto finish;
            
    }
    // 所有目录块已经被填满
    if (found == 0)  rc = -1;
    
finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    if (found == 0)
        Debug("no free directory found and no direct blocks available\n");
    return rc;
}



/* 通过文件名或路径名在目录中查找索引节点 */
int Find_InodeInDirectory(struct GOSFS_Instance *p_instance, char *path, ulong_t searchInode, ulong_t* retInode)
{
    ulong_t i = 0, e = 0, blockNum;
    int rc = -1, ret = -1;
    struct FS_Buffer *p_buff = 0;
    struct GOSFS_Directory    *dirEntry;

    Debug("Find_InodeInDirectory: inode=%d path=%s\n",(int)searchInode, path);
    // 查询所有直接块
    for (i=0; i<GOSFS_NUM_DIRECT_BLOCKS; i++)
    {
        blockNum = p_instance->superblock.inodes[searchInode].blockList[i];
        if (blockNum != 0)
        {
            rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
            // search through all directory entries
            for (e=0; e < GOSFS_DIR_ENTRIES_PER_BLOCK; e++)
            {
                dirEntry = (struct GOSFS_Directory*)((p_buff->data)+(e*sizeof(struct GOSFS_Directory)));
                if (dirEntry->type != GOSFS_DIRTYP_FREE)
                {
                    if (strcmp(dirEntry->filename, path)==0)
                    {
                        *retInode = dirEntry->inode;
                        ret = 0;
                        goto finish;
                    }
                }
            }
            Release_FS_Buffer(p_instance->buffercache, p_buff);
            p_buff=0;
        }
    }
    
finish:
    if (p_buff != 0) Release_FS_Buffer(p_instance->buffercache, p_buff);

    if (ret < 0) 
    {
        Debug("Find_InodeInDirectory: inode not found: %d\n",ret);
        return ret;
    }
    else 
    {
        Debug("Find_InodeInDirectory: returns %d\n",(int)*retInode);
        return ret;
    }
}

//通过给定的文件路径寻找inode
int Find_InodeByName(struct GOSFS_Instance *p_instance, const char *path, ulong_t* retInode)
{
    char* searchPath;
    char* nextSlash;
    int offset, rc = 0, finished = 0;
    ulong_t inode = 0;
    
    searchPath = Malloc(strlen(path)+1);
    
    Debug("Find_InodeByName: path=%s\n", path);
    
    // assume root-directory
    if (strcmp(path,"") == 0)
    {
        *retInode = 0;
        return rc;
    }
    if (path[0]!='/')  return -2;
    offset = 1;
    
    nextSlash=strchr(path+offset,'/');

    while (finished == 0)
    {
        if (nextSlash == 0)
        {
            finished = 1;
            nextSlash = (char*)path+strlen(path);
            
        }
        
        searchPath = strncpy(searchPath, path+offset, nextSlash-(path+offset));
        searchPath[nextSlash-(path+offset)]='\0';
        Debug("searching for part %s in inode %d\n",searchPath, (int)inode);
        
        rc = Find_InodeInDirectory(p_instance, searchPath, inode, &inode);
        if (rc < 0) break;
        
        offset=nextSlash-path+1;
        nextSlash=strchr(path+offset,'/');
    }
    
    Free(searchPath);
    
    *retInode=inode;
    Debug("found inode %d\n",(int) inode);
    return rc;
}

/* 创建一个文件inode */
int CreateFileInode(struct Mount_Point *mountPoint, const char *path, ulong_t *inode)
{
    int rc=0;
    struct GOSFS_Inode* pInode;
    struct GOSFS_Instance* p_instance = (struct GOSFS_Instance*) mountPoint->fsData;
    struct GOSFS_Directory    dirEntry;
    char *filename=0, *parentpath=0;
    ulong_t parentInode;
    
    // strip filename from path including /
    filename=strrchr(path,'/')+1;
    
    rc = Find_Free_Inode(mountPoint, inode);
    if (rc<0)
    {
        rc=EFSGEN;
        goto finish;
    }
    
    pInode=&(p_instance->superblock.inodes[*inode]);
    pInode->inode=*inode;
    pInode->link_count=1;
    pInode->flags=GOSFS_INODE_USED;
    memset (pInode->acl, '\0', sizeof (struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);
    pInode->acl[0].uid = g_currentThread->userContext ? g_currentThread->userContext->eUId : 0;
    pInode->acl[0].permission = O_READ | O_WRITE;
    
    dirEntry.type=GOSFS_DIRTYP_REGULAR;
    dirEntry.inode=*inode;
    strcpy(dirEntry.filename, filename);
    
    parentpath=Malloc(strlen(path));
    strncpy(parentpath, path, strrchr(path,'/')-path);
    parentpath[strrchr(path,'/')-path]='\0';
    Debug("searching for inode of parent path %s\n",parentpath);
    
    // 搜索根节点
    rc=Find_InodeByName(p_instance, parentpath, &parentInode);
    if (rc<0)
    {
        Debug("parent inode not found\n");
        rc=EFSGEN;
        goto finish;
    }
    
    rc=AddDirEntry2Inode(p_instance, parentInode, &dirEntry);
    if (rc<0)
    {
        Debug("failed to create directory-entry\n");
        rc=EFSGEN;
        goto finish;
    }

finish:
    return rc;
}

/* 检查是否为特定文件分配了特定的块号（此文件的第x个块） */
bool FileBlockExists(struct GOSFS_Instance *p_instance, struct GOSFS_Inode* inode, ulong_t blockNum)
{
    int rc=0;
    struct FS_Buffer* p_buff=0;
    int numIndirectPtr=-1; // 1-based
    int numPtrInIndirect=-1, numPtrIn2Indirect=-1; // 0-based
    int inodePtr=-1; // 0-based
    ulong_t indirectBlock=0;
    ulong_t phyBlock=0, phyIndBlock=0;
    
    // 检查块号是否在范围内 
    if (blockNum >= GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+(GOSFS_NUM_2X_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK*GOSFS_NUM_PTRS_PER_BLOCK)) 
    {
        rc=0;
        goto finish;
    }
    
    // 检查直接块
    if (blockNum < GOSFS_NUM_DIRECT_BLOCKS)
    {
        if (inode->blockList[blockNum] != 0)
            rc = 1;
            
    }
    // 检查间接块
    else if (blockNum < GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK))
    {
        
        numIndirectPtr = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+1)) / GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)) + 1;
        numPtrInIndirect = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+1)) % GOSFS_NUM_INDIRECT_PTR_PER_BLOCK));
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + numIndirectPtr -1;
        
        indirectBlock=inode->blockList[inodePtr];
        if (indirectBlock==0)
        {
            rc=0;
            goto finish;
        }
        
        rc = Get_FS_Buffer(p_instance->buffercache,indirectBlock,&p_buff);
		memcpy(&phyBlock, p_buff->data + (numPtrInIndirect*sizeof(ulong_t)), sizeof(ulong_t));
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
        if (phyBlock>0) rc=1;
    }
	// 检查二次间接块
	else
	{
		numIndirectPtr = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK*GOSFS_NUM_INDIRECT_PTR_PER_BLOCK))) + 1;
        numPtrIn2Indirect = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
		numPtrInIndirect = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) % (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK) + numIndirectPtr -1;
		
		indirectBlock=inode->blockList[inodePtr];
        if (indirectBlock==0)
        {
            rc=0;
            goto finish;
        }
        
        rc = Get_FS_Buffer(p_instance->buffercache,indirectBlock,&p_buff);
		memcpy(&phyIndBlock, p_buff->data + (numPtrIn2Indirect*sizeof(ulong_t)), sizeof(ulong_t));
        
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
		if (phyIndBlock<=0)
		{
			rc=0;
			goto finish;
		}
		rc = Get_FS_Buffer(p_instance->buffercache,phyIndBlock,&p_buff);
        memcpy(&phyBlock, p_buff->data + (numPtrInIndirect*sizeof(ulong_t)), sizeof(ulong_t));
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
        if (phyBlock>0) rc=1;

	}
    
finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    return rc;    
}

/* 创建一个新的空闲块*/
ulong_t GetNewCleanBlock(struct GOSFS_Instance *p_instance)
{
    ulong_t freeBlock;
    int rc=0;
    struct FS_Buffer *p_buff=0;
    
    freeBlock=Find_First_Free_Bit(p_instance->superblock.bitSet, p_instance->superblock.size);
    Debug("found free block %ld\n",freeBlock);
    if (freeBlock<=0)
    {
    Debug("No free Blocks found\n");
        rc=EFSGEN;
        goto finish;
    }
    
    rc = Get_FS_Buffer(p_instance->buffercache,freeBlock,&p_buff);
    // lets "format" block
    memset(p_buff->data,'\0',GOSFS_FS_BLOCK_SIZE);
    Modify_FS_Buffer(p_instance->buffercache,p_buff);
    rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
    p_buff = 0;
    Set_Bit(p_instance->superblock.bitSet, freeBlock);

finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    if (rc<0) return rc;
    else return freeBlock;
}


/* 写入间接块  */
int WriteIndirectBlockEntry(struct GOSFS_Instance *p_instance, ulong_t numBlock, ulong_t offset, ulong_t freeBlock)
{
    int rc=0;
    struct FS_Buffer *p_buff=0;
        
    rc = Get_FS_Buffer(p_instance->buffercache,numBlock,&p_buff);
    // lets "format" block
    memcpy(p_buff->data + (offset * sizeof(ulong_t)), &freeBlock, sizeof(ulong_t));
    
    Modify_FS_Buffer(p_instance->buffercache,p_buff);
    rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
    p_buff = 0;

finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    return rc;
    
}

/* 通过块的线性地址得到块的物理地址 */
int GetPhysicalBlockByLogical(struct GOSFS_Instance* p_instance, struct GOSFS_Inode* inode, ulong_t blockNum)
{
    int rc=0;
    int phyBlock=0, phyIndBlock=0;
    int numIndirectPtr = -1;    // which indirect inode ptr to use (1-based) 
    int numPtrInIndirect = -1; // which pointer in the indirct-block (0-based) 一次间接块
	int numPtrIn2Indirect = -1; // which pointer in the 2xindirct-block (0-based) 二次间接块
    int inodePtr = -1;    // which entry in the inode-array are we refering to (0-based)
    int indirectBlock;  // physical block with block-ptrs
    struct FS_Buffer *p_buff=0;
    
    // 直接块
    if (blockNum < GOSFS_NUM_DIRECT_BLOCKS)
    {
        phyBlock = inode->blockList[blockNum];
    }
    // 间接块
    else if (blockNum < GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK * GOSFS_NUM_INDIRECT_BLOCKS))
    {
        numIndirectPtr = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+1)) / GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)) + 1;
        numPtrInIndirect = (((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+1)) % GOSFS_NUM_INDIRECT_PTR_PER_BLOCK);
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + numIndirectPtr -1;
        Debug("GetPhysicalBlockByLogical: blocknum: %ld, numIndirectPtr: %d, numPtrInIndirect: %d, inodePtr: %d\n",blockNum, numIndirectPtr,numPtrInIndirect,inodePtr);
        
        indirectBlock = inode->blockList[inodePtr];
        if (indirectBlock == 0)
        {
            Debug("indirect pointer not initialized\n");
            rc = EFSGEN;
            goto finish;
        }
        
        rc = Get_FS_Buffer(p_instance->buffercache,indirectBlock,&p_buff);

        memcpy(&phyBlock, p_buff->data + (numPtrInIndirect*sizeof(ulong_t)), sizeof(ulong_t));

        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;

    }
	// 二次间接块
	else
	{
		numIndirectPtr = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK*GOSFS_NUM_INDIRECT_PTR_PER_BLOCK))) + 1;
        numPtrIn2Indirect = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
		numPtrInIndirect = ((((blockNum+1) - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) % (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK) + numIndirectPtr -1;
		Debug("GetPhysicalBlockByLogical: blocknum: %ld, numIndirectPtr: %d, numPtrIn2Indirect: %d, numPtrInIndirect: %d, inodePtr: %d\n",blockNum, numIndirectPtr,numPtrIn2Indirect,numPtrInIndirect,inodePtr);
	
        indirectBlock = inode->blockList[inodePtr];
        if (indirectBlock == 0)
        {
            Debug("indirect pointer not initialized\n");
            rc = EFSGEN;
            goto finish;
        }
        
        rc = Get_FS_Buffer(p_instance->buffercache,indirectBlock,&p_buff);
      
        memcpy(&phyIndBlock, p_buff->data + (numPtrIn2Indirect*sizeof(ulong_t)), sizeof(ulong_t));

        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
        rc = Get_FS_Buffer(p_instance->buffercache,phyIndBlock,&p_buff);
       
        memcpy(&phyBlock, p_buff->data + (numPtrInIndirect*sizeof(ulong_t)), sizeof(ulong_t));

        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
     
		
	}
    
finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    if (rc<0) return rc;
    else return phyBlock;
}

/* 为inode指向的文件分配一个新块 */
int CreateFileBlock(struct GOSFS_Instance* p_instance, struct GOSFS_Inode* inode, ulong_t blockNum)
{
    int rc=0;
    int freeBlock;
    struct FS_Buffer *p_buff=0;
    int numIndirectPtr = -1;    // which indirect inode ptr to use (1-based)
    int numPtrInIndirect = -1; // which pointer in the indirct-block (0-based)
	int numPtrIn2Indirect = -1; // which pointer in the 2xindirct-block (0-based)
    int inodePtr = -1;    // which entry in the inode-array are we refering to (0-based)
    int indirectBlock;  // physical block with block-ptrs
	ulong_t phyIndBlock=-1;
    
    blockNum++; // lets start by 1 here, not 0-based
    // create block to store data in
    freeBlock=GetNewCleanBlock(p_instance);
    if (freeBlock<=0)
    {
        Debug("No free Blocks found\n");
        rc=EFSGEN;
        goto finish;
    }
    
    // 查看需要哪种类型的块，直接块，间接块或间接块 
    // direct block
    if (blockNum <= GOSFS_NUM_DIRECT_BLOCKS)
    {
        Debug("using direct pointer\n");
        inodePtr = blockNum -1;
        inode->blockList[inodePtr]=freeBlock;
    }
    // 间接块
    else if (blockNum <= (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK * GOSFS_NUM_INDIRECT_BLOCKS )+ GOSFS_NUM_DIRECT_BLOCKS)
    {
        Debug("using indirect pointer\n");
        numIndirectPtr = (((blockNum - (GOSFS_NUM_DIRECT_BLOCKS+1)) / GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)) + 1;
        numPtrInIndirect = (((blockNum - (GOSFS_NUM_DIRECT_BLOCKS+1)) % GOSFS_NUM_INDIRECT_PTR_PER_BLOCK));
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + numIndirectPtr -1;
        
        // 如果这是间接块的首次使用，则需要对其进行初始化 
        indirectBlock = inode->blockList[inodePtr];
        if (indirectBlock == 0)
        {
            indirectBlock = GetNewCleanBlock(p_instance);
         
            Debug("setting inode blocklistindex %d inodePtr to block %d\n",inodePtr,indirectBlock);
            inode->blockList[inodePtr]=    indirectBlock;    
        }
        // 写入间接块
        rc = WriteIndirectBlockEntry(p_instance, indirectBlock, numPtrInIndirect, freeBlock);
         
        
    }
	// 二次间接块
    else if (blockNum <= (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK*GOSFS_NUM_PTRS_PER_BLOCK) + (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK * GOSFS_NUM_INDIRECT_BLOCKS )+ GOSFS_NUM_DIRECT_BLOCKS)
    {
        Debug("using 2Xindirect pointer\n");
		numIndirectPtr = (((blockNum - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK*GOSFS_NUM_INDIRECT_PTR_PER_BLOCK))) + 1;
        numPtrIn2Indirect = (((blockNum - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) / (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
		numPtrInIndirect = (((blockNum - (GOSFS_NUM_DIRECT_BLOCKS+(GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK)+1)) % (GOSFS_NUM_INDIRECT_PTR_PER_BLOCK)));
        inodePtr = GOSFS_NUM_DIRECT_BLOCKS + (GOSFS_NUM_INDIRECT_BLOCKS*GOSFS_NUM_PTRS_PER_BLOCK) + numIndirectPtr -1;

		// 如果这是二次间接块的首次使用，则需要对其进行初始化 
        indirectBlock = inode->blockList[inodePtr];
        if (indirectBlock == 0)
        {
            indirectBlock = GetNewCleanBlock(p_instance);
         
            Debug("setting inode 2xblocklistindex %d inodePtr to block %d\n",inodePtr,indirectBlock);
            inode->blockList[inodePtr]=    indirectBlock;    
        }
		
		rc = Get_FS_Buffer(p_instance->buffercache,indirectBlock,&p_buff);
        memcpy(&phyIndBlock, p_buff->data + (numPtrIn2Indirect*sizeof(ulong_t)), sizeof(ulong_t));
        rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
        p_buff = 0;
      
		// 如果block还没有被分配
		if (phyIndBlock<=0)
		{
			phyIndBlock = GetNewCleanBlock(p_instance);
            if (phyIndBlock<=0)
            {
                Debug("No free Blocks found for 2xindirect block\n");
                rc=EFSGEN;
                goto finish;
            }
			// 写入二次间接块
        	rc = WriteIndirectBlockEntry(p_instance, indirectBlock, numPtrIn2Indirect, phyIndBlock);
        	if (rc<0)
        	{
            	Debug("could not write entry to indirect block %d\n",rc);
            	rc = EFSGEN;
            	goto finish;
        	}
			
		}
		
		// 写入二次间接块
        rc = WriteIndirectBlockEntry(p_instance, phyIndBlock, numPtrInIndirect, freeBlock);
	}
    else
    {
        Debug("maximum filesize reached\n");
        rc=EMAXSIZE;
        goto finish;
    }
        
    inode->blocks_used++;
    
finish:
    if (p_buff!=0) Release_FS_Buffer(p_instance->buffercache, p_buff);
    return rc;
}


/*
 * 为给定的文件得到元数据
 */
static int GOSFS_FStat(struct File *file, struct VFS_File_Stat *stat)
{
    int rc=0;
    struct GOSFS_FileEntry* fileEntry = (struct GOSFS_FileEntry*) file->fsData;
    
    Mutex_Lock(&fileEntry->instance->lock);
    
    stat->size = fileEntry->inode->size;
    
    if (fileEntry->inode->flags & GOSFS_INODE_ISDIRECTORY)
        stat->isDirectory = 1;
    else
        stat->isDirectory = 0;
    memcpy (stat->acls, fileEntry->inode->acl,
            sizeof(struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);
    Mutex_Unlock(&fileEntry->instance->lock);

    return rc;
}

/*
 * 从给定文件的当前位置读数据
 */
static int GOSFS_Read(struct File *file, void *buf, ulong_t numBytes)
{

    int rc=0;
    struct GOSFS_FileEntry* pFileEntry= (struct GOSFS_FileEntry*) file->fsData;
    ulong_t offset = file->filePos;
    ulong_t readTo = (file->filePos+numBytes-1);
    struct FS_Buffer* p_buff=0;
    ulong_t startBlock = offset / GOSFS_FS_BLOCK_SIZE;
    ulong_t endBlock = readTo / GOSFS_FS_BLOCK_SIZE;
    ulong_t i=0;
    ulong_t phyBlock, readFrom=0, readNum=0, bytesRead=0;


    Debug ("GOSFS_Read: about to read from offs=%ld (startblk=%ld) to end=%ld (endblk=%ld)\n",
           offset, startBlock, readTo, endBlock);

    Mutex_Lock(&pFileEntry->instance->lock);
    
    // check if read operation is allowed
    if (!(file->mode & O_READ))
    {
        Debug("trying to read from wirte-only file\n");
        rc=EACCESS;
        goto finish;
    }
    
    Debug ("filepos: %ld, endpos: %ld\n",file->filePos, file->endPos);
    // check if we are at the end of the file
    if (file->filePos >= file->endPos)
    {
        bytesRead=0;
        goto finish;
    }        
    for (i=startBlock; i<=endBlock; i++)
    {
        //phyBlock = pFileEntry->inode->blockList[i];
        phyBlock = GetPhysicalBlockByLogical(pFileEntry->instance, pFileEntry->inode, i);

        if (phyBlock == 0)
        {
            Debug("block not allocated\n");
            rc = EFSGEN;
            goto finish;
        }
        
        // read data
        rc = Get_FS_Buffer(pFileEntry->instance->buffercache,phyBlock,&p_buff);
     
        if (i==startBlock)
            readFrom=offset % GOSFS_FS_BLOCK_SIZE;
        else
            readFrom=0;
        
        readNum = GOSFS_FS_BLOCK_SIZE-readFrom;
        if (bytesRead+readNum > numBytes) readNum = numBytes - bytesRead;
        if (readNum + bytesRead + offset > file->endPos)
            readNum = file->endPos-offset-bytesRead;
        memcpy(buf + bytesRead, p_buff->data + readFrom, readNum);
        bytesRead = bytesRead + readNum;

        
        rc = Release_FS_Buffer(pFileEntry->instance->buffercache, p_buff);
        p_buff = 0;
        if (rc<0)
        {
            Debug("Unable to release fs_buffer\n");
            rc = EFSGEN;
            goto finish;
        }
    }
    file->filePos=file->filePos+numBytes;
    
finish:
    Debug ("numBytesRead = %ld\n",bytesRead);
    if (p_buff!=0) Release_FS_Buffer(pFileEntry->instance->buffercache, p_buff);
    Mutex_Unlock(&pFileEntry->instance->lock);
    if (rc<0) return rc;
    else return bytesRead;
}

/*
 *将数据写入文件中的当前位置 
 */
static int GOSFS_Write(struct File *file, void *buf, ulong_t numBytes)
{
    //TODO("GeekOS filesystem write operation");
    int rc=0;
    //ulong_t blocksNeeded = 0;
    ulong_t startBlock = 0;
    ulong_t startBlockOffset = 0;
    ulong_t endBlock = 0;
    ulong_t i = 0;
    struct GOSFS_FileEntry* pFileEntry = file->fsData;
    struct FS_Buffer* p_buff = 0;
    ulong_t phyBlock, writeFrom, writeNum, bytesWritten = 0;
    struct Mutex* lock = &pFileEntry->instance->lock;
    
    Debug("GOSFS_Write: about to write %ld bytes at offset %ld\n", numBytes, file->filePos);
    
    Mutex_Lock(lock);
    
    // 检查写操作是否被允许
    if (!(file->mode & O_WRITE))
    {
        Debug("trying to write from read-only file\n");
        rc = EACCESS;
        goto finish;
    }
    
    // 计算需要写入的数据块
    startBlock = file->filePos / GOSFS_FS_BLOCK_SIZE;
    startBlockOffset = file->filePos % GOSFS_FS_BLOCK_SIZE;
    endBlock = (file->filePos + numBytes) / GOSFS_FS_BLOCK_SIZE;

    Debug("logical blocks %ld - %ld needed\n",startBlock, endBlock);
    
    // write data to disk
    for (i=startBlock; i<=endBlock; i++)
    {
        // check block for existence, otherwise allocate block
        if (!FileBlockExists(pFileEntry->instance, pFileEntry->inode, i))
        {
            Debug("block not allocated --> allocate new block\n");
            rc=CreateFileBlock(pFileEntry->instance, pFileEntry->inode, i);
            if (rc<0)
            {
                Debug("received errorcode %d from CreateFileBlock\n",rc);
                goto finish;
            }
        }
        
        //phyBlock = pFileEntry->inode->blockList[i];
        phyBlock = GetPhysicalBlockByLogical(pFileEntry->instance, pFileEntry->inode, i);
        if (phyBlock<=0)
        {
            Debug("block not allocated \n");
            rc = EFSGEN;
            goto finish;
        }
        
        Debug("About to write (logical) blocknumber %ld to physical block %ld\n",i,phyBlock);
        
        // write data
        rc = Get_FS_Buffer(pFileEntry->instance->buffercache,phyBlock,&p_buff);
        if (rc<0)
        {
            Debug("Unable to get buffer\n");
            rc = EFSGEN;
            goto finish;
        }
        
        if (i == startBlock) writeFrom=startBlockOffset;
        else writeFrom = 0;
            
        writeNum = GOSFS_FS_BLOCK_SIZE - writeFrom;
        
        if (writeNum > numBytes-bytesWritten) writeNum=numBytes-bytesWritten;
        Debug("writeFrom=%ld, writeNum=%ld\n",writeFrom, writeNum);
        
        memcpy(p_buff->data+writeFrom, buf+bytesWritten, writeNum);
        bytesWritten = bytesWritten + writeNum;
        Modify_FS_Buffer(pFileEntry->instance->buffercache,p_buff);
        rc = Release_FS_Buffer(pFileEntry->instance->buffercache, p_buff);
        p_buff = 0;

    }
    
    // 使inode信息和文件描述符保持最新 
    if (file->filePos + numBytes > pFileEntry->inode->size)
    {
        pFileEntry->inode->size = file->filePos + numBytes;
        file->endPos=pFileEntry->inode->size;
    }
    file->filePos=file->filePos + numBytes;

finish:
    if (p_buff != 0) Release_FS_Buffer(pFileEntry->instance->buffercache, p_buff);
    Mutex_Unlock(lock);
    if (rc < 0) return rc;
    else return bytesWritten;
}

/*
 * 文件指针定位
 */
static int GOSFS_Seek(struct File *file, ulong_t pos)
{
    //TODO("GeekOS filesystem seek operation");
    int rc = 0;
    file->filePos = pos;
    return rc;
}

/*
 * 关闭文件
 */
static int GOSFS_Close(struct File *file)
{
    struct GOSFS_FileEntry * pFileEntry = file->fsData;
    struct GOSFS_Instance* p_instance = pFileEntry->instance;

   if (!pFileEntry)
        return 0;
    --pFileEntry->references;
    if (pFileEntry->references)
        return 0;
    
    Mutex_Lock (&p_instance->lock);
    Free (pFileEntry);
    Mutex_Unlock (&p_instance->lock);

    return 0;
}


/*
  *克隆文件。
  *克隆将访问与以下文件相同的基础文件
  *原始的，但读/写/查找操作等将有所不同。
  *如果成功则返回0，否则返回错误代码。 
*/
static int GOSFS_Clone(struct File *file, struct File **pClone)
{
    //TODO("GeekOS filesystem clone operation");
    int rc = 0;
    struct File *clone;
    struct GOSFS_FileEntry* pFileEntry = 0;

    /* 创建一个重复的File对象*/
    clone = Allocate_File (file->ops, file->filePos, file->endPos, file->fsData,
                           file->mode, file->mountPoint);
    if (clone == 0) {
        rc = ENOMEM;
        goto done;
    }

    *pClone = clone;

    pFileEntry = (struct GOSFS_FileEntry*) file->fsData;
    bool iflag = Begin_Int_Atomic();
    ++pFileEntry->references;
    End_Int_Atomic(iflag);

done:
    Print ("GOSFS_Clone: pid=%d ref=%d, file=%p, clone=%p\n",
           g_currentThread->pid, pFileEntry->references, file, clone);
    return rc;
}

static struct File_Ops s_gosfsFileOps = {
    &GOSFS_FStat,
    &GOSFS_Read,
    &GOSFS_Write,
    &GOSFS_Seek,
    &GOSFS_Close,
    0, /* Read_Entry */
    &GOSFS_Clone,
};

/*
 * stat已经打开的目录
 */
static int GOSFS_FStat_Directory(struct File *dir, struct VFS_File_Stat *stat)
{
    int rc=0;
    struct GOSFS_FileEntry* fileEntry = 0; // (struct GOSFS_FileEntry*) dir->fsData;

    stat->size=dir->endPos;
    stat->isDirectory=1;
    stat->isSetuid=0;
    
    if (!fileEntry)
        Print ("ERROR: GOSFS_FStat_Directory: fsData == NULL!\n");
    else
        memcpy (stat->acls, fileEntry->inode->acl,
                sizeof(struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);
    return rc;
}

/*
 * 关闭目录操作
 */
static int GOSFS_Close_Directory(struct File *dir)
{
    //TODO("GeekOS filesystem Close directory operation");
    struct GOSFS_Directory *dirEntries = 0;

    if (!dir)  return EINVALID;
    dirEntries = (struct GOSFS_Directory*)dir->fsData;
    if (!dirEntries)  return EINVALID;
    Free(dirEntries);
    //Free(dir);     // file itself is freed else where in vfs
    return 0;
}

/*
 *从打开的目录中读取目录项
 */
static int GOSFS_Read_Entry(struct File *dir, struct VFS_Dir_Entry *entry)
{
    //TODO("GeekOS filesystem Read_Entry operation");
    int rc=0;
    ulong_t offset = dir->filePos;
    struct GOSFS_Directory *directory;
    struct GOSFS_Inode *inode;
        
    if (dir->filePos >= dir->endPos)
        return VFS_NO_MORE_DIR_ENTRIES;    // we are at the end of the file
    
    directory = ((struct GOSFS_Directory*) dir->fsData)+offset;
    strcpy(entry->name, directory->filename);
    inode = &(((struct GOSFS_Instance*)(dir->mountPoint->fsData))->superblock.inodes[directory->inode]);
    entry->stats.size = inode->size;
    entry->stats.isDirectory = (inode->flags & GOSFS_INODE_ISDIRECTORY) ? 1 : 0;
    entry->stats.isSetuid    = (inode->flags & GOSFS_INODE_SETUID     ) ? 1 : 0;
    dir->filePos++;    // increase file pos

    memcpy (entry->stats.acls, inode->acl,
            sizeof(struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);

    return rc;
}

static struct File_Ops s_gosfsDirOps = {
    &GOSFS_FStat_Directory,
    0, /* Read */
    0, /* Write */
//  0, /* Seek */
    &GOSFS_Seek,
    &GOSFS_Close_Directory,
    &GOSFS_Read_Entry,
};

/*
 * 打开一个由给定路径命名的文件
 */
static int GOSFS_Open(struct Mount_Point *mountPoint, const char *path, int mode, struct File **pFile)
{
    //TODO("GeekOS filesystem open operation");
    int rc=0;
    struct GOSFS_Instance *p_instance = (struct GOSFS_Instance*) mountPoint->fsData;
    ulong_t inode;
    struct GOSFS_Inode*    pInode;
    struct GOSFS_FileEntry* pFileEntry=0;
    
    Debug ("GOSFS_Open: path=%s, mode=%d\n",path, mode);
    
    Mutex_Lock(&p_instance->lock);
    
    // check if file already exists
    rc=Find_InodeByName(p_instance, path, &inode);
    if (rc<0)
    {
        Debug ("file not found\n");
        if (!(mode & O_CREATE))
        {
            rc=ENOTFOUND;
            goto finish;
        }
        Debug ("about to create file\n");
        
        rc = CreateFileInode(mountPoint, path, &inode);
        
        if (rc<0)
        {
            Debug ("file could not be created\n");
            rc=EFSGEN;
            goto finish;
        }
    }

    pInode = &p_instance->superblock.inodes[inode];
    pFileEntry = Malloc(sizeof(struct GOSFS_FileEntry));
    if (pFileEntry == 0)
    {
        rc = ENOMEM;
        goto finish;
    }
    pFileEntry->inode = pInode;
    pFileEntry->instance = p_instance;
    pFileEntry->references = 1;
    
    struct File *file = Allocate_File(&s_gosfsFileOps, 0, pInode->size, pFileEntry, mode, mountPoint);
    if (file == 0) {
        rc = ENOMEM;
        goto finish;
    }

    *pFile = file;
    
    Debug ("GOSFS_Open: pid=%d ref=%d, fda=%p\n",
          g_currentThread->pid, pFileEntry->references, pFileEntry);

finish:
    if ((rc<0) && (pFileEntry!=0)) Free(pFileEntry);
    Mutex_Unlock(&p_instance->lock);
    return rc;
}

/*
 * 创建一个指定路径的目录
 */
static int GOSFS_Create_Directory(struct Mount_Point *mountPoint, const char *path)
{
    int rc=0,e;
    ulong_t freeBlock;
    struct GOSFS_Directory    dirEntry;
    struct FS_Buffer            *p_buff=0;
    struct GOSFS_Instance        *p_instance;
    ulong_t freeInode, parentInode, tmpInode;
    char*                filename=0;
    char*                parentpath=0;
    
    p_instance = (struct GOSFS_Instance*) mountPoint->fsData;    
        
    Debug("about to create directory %s\n",path);
    
    Mutex_Lock(&p_instance->lock);
    
    parentpath=Malloc(strlen(path));
    strncpy(parentpath, path, strrchr(path,'/')-path);
    parentpath[strrchr(path,'/')-path]='\0';
    Debug("searching for inode of parent path %s\n",parentpath);
    
    // 从路径中删除文件名
    rc=Find_InodeByName(p_instance, parentpath, &parentInode);
    
    rc=Find_Free_Inode(mountPoint, &freeInode);
    Debug("found free inode %d\n",(int)freeInode);
   

    // strip filename from path including /
    filename=strrchr(path,'/')+1;
    
    // 检查是否已经存在
    rc=Find_InodeInDirectory(p_instance, filename, parentInode, &tmpInode);
    // 写入 directory entry 在 父 inode
    dirEntry.type = GOSFS_DIRTYP_REGULAR;
    dirEntry.inode = freeInode;
    strcpy(dirEntry.filename, filename);
    rc = AddDirEntry2Inode(p_instance, parentInode, &dirEntry);
    freeBlock=Find_First_Free_Bit(p_instance->superblock.bitSet, p_instance->superblock.size);
    rc = Get_FS_Buffer(p_instance->buffercache,freeBlock,&p_buff);
    rc = CreateFirstDirectoryBlock(freeInode, 0, p_buff);
    Modify_FS_Buffer(p_instance->buffercache,p_buff);
    rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
    p_buff = 0;
    Set_Bit(p_instance->superblock.bitSet, freeBlock);

    p_instance->superblock.inodes[freeInode].size=2;        // directories start with 2 entries ("." and "..")
    p_instance->superblock.inodes[freeInode].link_count=1;
    p_instance->superblock.inodes[freeInode].blocks_used=1; // new directories will consume 1 Block
    p_instance->superblock.inodes[freeInode].flags=GOSFS_INODE_ISDIRECTORY | GOSFS_INODE_USED;
    memset (p_instance->superblock.inodes[freeInode].acl, '\0', sizeof (struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);
    
    p_instance->superblock.inodes[freeInode].blockList[0]=freeBlock;
    
finish:
    if (parentpath!=0) Free(parentpath);
    Mutex_Unlock(&p_instance->lock);
    return rc;
}

/*
 * 通过指定路径打开文件夹
 */
static int GOSFS_Open_Directory(struct Mount_Point *mountPoint, const char *path, struct File **pDir)
{
    int rc = 0, i, e;
    ulong_t inodeNum, blockNum, found = 0;
    struct GOSFS_Inode    *inode = 0;
    struct GOSFS_Directory *dirEntries = 0, *tmpDir;
    struct FS_Buffer *p_buff = 0;
    struct GOSFS_Instance* p_instance = (struct GOSFS_Instance*)mountPoint->fsData;
    
    Mutex_Lock(&p_instance->lock);
    
    // spezial treatment for root-directory
    if (strcmp(path,"/") == 0)
    {
        inodeNum=0;
    }
    else
    {
        rc = Find_InodeByName((struct GOSFS_Instance*)mountPoint->fsData, path, &inodeNum);
        if (rc < 0)  goto finish;
    }
        
    inode = &(((struct GOSFS_Instance*)mountPoint->fsData)->superblock.inodes[inodeNum]);
    
    (*pDir)->ops = &s_gosfsDirOps;
    (*pDir)->filePos = 0;
    (*pDir)->endPos = inode->size;
    (*pDir)->mode = O_READ;
    (*pDir)->mountPoint=mountPoint;
    
    dirEntries = Malloc(inode->size * sizeof(struct GOSFS_Directory));
    if (dirEntries == 0)
    {
        rc = ENOMEM;
        goto finish;
    }
    
    // put directory listing in file-data
    for (i=0; i < GOSFS_NUM_DIRECT_BLOCKS; i++)
    {        
        blockNum = inode->blockList[i];
        if (blockNum != 0)
        {
            Debug("found direct block %ld\n",blockNum);
            rc = Get_FS_Buffer(((struct GOSFS_Instance*)mountPoint->fsData)->buffercache,blockNum,&p_buff);
          
            for (e = 0; e < GOSFS_DIR_ENTRIES_PER_BLOCK; e++)
            {
                tmpDir = (struct GOSFS_Directory*)((p_buff->data)+(e*sizeof(struct GOSFS_Directory)));
                if (tmpDir->type != GOSFS_DIRTYP_FREE)
                {
                    Debug("found directory entry %d\n",e);
                    memcpy(dirEntries+found, tmpDir, sizeof(struct GOSFS_Directory));
                    found++;
                }
            }
            
            rc = Release_FS_Buffer(((struct GOSFS_Instance*)mountPoint->fsData)->buffercache, p_buff);
            p_buff = 0;         
        }
    }
    (*pDir)->fsData = dirEntries;
        
    Debug ("GOSFS_Open_Directory: pid=%d fda=%p\n",
          g_currentThread->pid, dirEntries);
    
finish:
    if ((rc < 0) && (dirEntries != 0))  Free(dirEntries);
    if (p_buff != 0)  Release_FS_Buffer(((struct GOSFS_Instance*)mountPoint->fsData)->buffercache, p_buff);
    Mutex_Unlock(&p_instance->lock);
    return rc;
}

/*
 * 删除由给定路径命名的文件或目录
 */
static int GOSFS_Delete(struct Mount_Point *mountPoint, const char *path)
{
    //TODO("GeekOS filesystem delete operation");
    int rc=0,i=0;
    ulong_t inodeNum=-1, parentInodeNum=-1, e=0, f=0;
    struct GOSFS_Inode *pInode=0;
    ulong_t blockNum=0, blockIndirect=0, block2Indirect=0;
    struct GOSFS_Instance *p_instance = (struct GOSFS_Instance*)mountPoint->fsData;
    char  *parentPath=0, *offset;
    struct FS_Buffer *p_buff=0;

    Mutex_Lock(&p_instance->lock);
    
    rc = Find_InodeByName(p_instance, path, &inodeNum);
    

    
    pInode = &(p_instance->superblock.inodes[inodeNum]);
    
    if (!IsDirectoryEmpty(pInode,p_instance))
    {
        Debug("seems to be non-empty directory\n");
        rc = EDIRNOTEMPTY;
        goto finish;        
    }
    
    // find parent directory
    offset = strrchr(path,'/');    
    parentPath = strdup(path);
    parentPath[offset-path]='\0';
    Debug("parent-path: %s\n",parentPath);
    rc = Find_InodeByName(p_instance, parentPath, &parentInodeNum);

    // free all asigned direct blocks of this inode
    for (i=0; i<GOSFS_NUM_DIRECT_BLOCKS; i++)
    {
        blockNum = pInode->blockList[i];
        if (blockNum != 0)
        {
            Clear_Bit(p_instance->superblock.bitSet, blockNum);
        }
    }
    
    // free indirect-blocks
    for (i=0; i<GOSFS_NUM_INDIRECT_BLOCKS; i++)
    {
        blockNum = pInode->blockList[GOSFS_NUM_DIRECT_BLOCKS+i];
        if (blockNum !=0)
        {
            Debug("found indirect block %ld --> freeing\n",blockNum);
            
            rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
          
            
            for (e=0; e<GOSFS_NUM_INDIRECT_PTR_PER_BLOCK; e++)
            {
                memcpy(&blockIndirect, (void*) p_buff->data + (e*sizeof(ulong_t)), sizeof(ulong_t));
                if (blockIndirect!=0)
                {
                    Debug("found block %ld to delete\n",blockIndirect);
                    Clear_Bit(p_instance->superblock.bitSet, blockIndirect);
                    
                }
            }
            
            rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
            p_buff = 0;          
            
            Clear_Bit(p_instance->superblock.bitSet, blockNum);
        }
    }
	
	// free 2Xindirect blocks
	for (i=0;i<GOSFS_NUM_2X_INDIRECT_BLOCKS;i++)
	{
		blockNum = pInode->blockList[GOSFS_NUM_DIRECT_BLOCKS+GOSFS_NUM_INDIRECT_BLOCKS+i];
        if (blockNum !=0)
        {
            Debug("found indirect block %ld --> freeing\n",blockNum);
			
            for (e=0; e<GOSFS_NUM_INDIRECT_PTR_PER_BLOCK; e++)
            {
				rc = Get_FS_Buffer(p_instance->buffercache,blockNum,&p_buff);
                memcpy(&block2Indirect, (void*) p_buff->data + (e*sizeof(ulong_t)), sizeof(ulong_t));
				rc = Release_FS_Buffer(p_instance->buffercache, p_buff);
            	p_buff = 0;
            }

            Clear_Bit(p_instance->superblock.bitSet, blockNum);		
			
		}
		
	}
    // remove directory-entry from parent directory
    rc = RemoveDirEntryFromInode(p_instance, parentInodeNum, inodeNum);
   
finish:
    if (p_buff!=0)  Release_FS_Buffer(((struct GOSFS_Instance*)mountPoint->fsData)->buffercache, p_buff);
    if (parentPath!=0) Free(parentPath);
    Mutex_Unlock(&p_instance->lock);
    return rc;
}

/*
 * 获取由给定路径命名的文件的元数据（大小，权限等）
 */
static int GOSFS_Stat(struct Mount_Point *mountPoint, const char *path, struct VFS_File_Stat *stat)
{        
    int rc=0;
    ulong_t inode=0;
    struct GOSFS_Instance *p_instance = (struct GOSFS_Instance*)mountPoint->fsData;
    Mutex_Lock(&p_instance->lock);    

    if (strcmp(path,"/")==0) 
    {
        inode=0;
    }
    else
    {
        rc=Find_InodeByName((struct GOSFS_Instance*)mountPoint->fsData, path, &inode);
        if (rc<0) 
        {
            rc=ENOTFOUND;
            goto finish;
        }
    }
    
    stat->size=((struct GOSFS_Instance*)mountPoint->fsData)->superblock.inodes[inode].size;
    
    if (!(((struct GOSFS_Instance*)mountPoint->fsData)->superblock.inodes[inode].flags & GOSFS_INODE_USED))
    {
        rc = ENOTFOUND;
        goto finish;
    }
    
    if (((struct GOSFS_Instance*)mountPoint->fsData)->superblock.inodes[inode].flags & GOSFS_INODE_ISDIRECTORY)
            stat->isDirectory=1;
    else 
            stat->isDirectory=0;

    memcpy (stat->acls, ((struct GOSFS_Instance*)mountPoint->fsData)->superblock.inodes[inode].acl,
            sizeof(struct VFS_ACL_Entry) * VFS_MAX_ACL_ENTRIES);
    
finish:
    Mutex_Unlock(&p_instance->lock);
    return rc;
}


/*
 * 将文件系统数据与磁盘同步 
 * (i.e., flush out all buffered filesystem data).
 */
static int GOSFS_Sync(struct Mount_Point *mountPoint)
{
    //TODO("GeekOS filesystem sync operation");
    int rc=0;
    //ulong_t i=0;
    struct GOSFS_Instance *p_instance = (struct GOSFS_Instance *)mountPoint->fsData;
    struct FS_Buffer    *p_buff=0;
    Mutex_Lock(&p_instance->lock);
    
    // we always need to write the superblock to disk
    rc = WriteSuperblock(p_instance);
    
finish:
    if (p_buff!=0)  Release_FS_Buffer(p_instance->buffercache, p_buff);
    Mutex_Unlock(&p_instance->lock);
    return rc;
}

static struct Mount_Point_Ops s_gosfsMountPointOps = {
    &GOSFS_Open,
    &GOSFS_Create_Directory,
    &GOSFS_Open_Directory,
    &GOSFS_Stat,
    &GOSFS_Sync,
    &GOSFS_Delete,

};
//将挂载区域blockDev进行GOSFS格式化
static int GOSFS_Format(struct Block_Device *blockDev)
{
    //TODO("GeekOS filesystem format operation");
    struct FS_Buffer_Cache        *gosfs_cache=0;
    struct FS_Buffer            *p_buff=0;
    struct GOSFS_Superblock        *superblock=0;
    int rc=0;
    uchar_t    *bitset=0;
    ulong_t bcopied=0;
    ulong_t i;
        
    
    int numBlocks = Get_Num_Blocks(blockDev)/GOSFS_SECTORS_PER_FS_BLOCK;
    
    ulong_t byteCountSuperblock = sizeof(struct GOSFS_Superblock) + FIND_NUM_BYTES(numBlocks);
    // 需要块的数量
    ulong_t blockCountSuperblock = FindNumBlocks(byteCountSuperblock);
    gosfs_cache = Create_FS_Buffer_Cache(blockDev, GOSFS_FS_BLOCK_SIZE);
    
    Debug("About to create root-directory\n");
    // create root directory entry
   
    // 建立超级块 
    superblock = Malloc(byteCountSuperblock);
    superblock->magic = GOSFS_MAGIC;
    superblock->size = numBlocks;
    superblock->supersize = byteCountSuperblock;
    
    for (i=0; i<GOSFS_NUM_INODES; i++)
    {
        superblock->inodes[i].inode=i;
    }
    // 将超级块写入硬盘 
    for (i=0; i<blockCountSuperblock; i++)
    {
        rc = Get_FS_Buffer(gosfs_cache, i, &p_buff) ;
        if ((byteCountSuperblock-bcopied) < GOSFS_FS_BLOCK_SIZE)
        {
            memcpy(p_buff->data, ((void*)superblock)+bcopied, byteCountSuperblock-bcopied);
            bcopied = bcopied+(byteCountSuperblock-bcopied);
        }
        else
        {
            memcpy(p_buff->data, ((void*)superblock)+bcopied, GOSFS_FS_BLOCK_SIZE);
            bcopied = bcopied + GOSFS_FS_BLOCK_SIZE;
        }
            Debug("Bytes written %ld\n",bcopied);
        
        Modify_FS_Buffer(gosfs_cache, p_buff);
    
    }

finish:
    return rc;
}

/**挂载mountPoint指向的文件系统**/
static int GOSFS_Mount(struct Mount_Point *mountPoint)
{
    Print("GeekOS filesystem mount operation");
    struct FS_Buffer_Cache        *gosfs_cache = 0;
    struct FS_Buffer            *p_buff = 0;
    struct GOSFS_Superblock        *superblock = 0;
    struct GOSFS_Instance        *instance;
    ulong_t numBlocks, numBytes,bwritten,i;
    int   rc;
    mountPoint->ops = &s_gosfsMountPointOps;
    gosfs_cache = Create_FS_Buffer_Cache(mountPoint->dev, GOSFS_FS_BLOCK_SIZE);
   
    // 超级块的第一个块
    rc = Get_FS_Buffer(gosfs_cache, 0, &p_buff) ;
    superblock = (struct GOSFS_Superblock*) p_buff->data;

    Print("found magic:%lx\n",superblock->magic);//魔数检查
    if (superblock->magic!=GOSFS_MAGIC)
    {
        Print("GOSFS_Mount ERROR: does not seem to be a GOSFS filesystem, try format first\n");
        rc = EFSGEN;
        goto finish;
    }
    Print("superblock size: %ld Byte\n",superblock->supersize);
    Print("number of blocks of whole fs %ld bocks\n",superblock->size);

    numBytes = superblock->supersize;
    numBlocks = (numBytes / GOSFS_FS_BLOCK_SIZE)+1;
        Print("superblock spreads %ld blocks\n",numBlocks);
    /* 创建文件系统实例 */
    int sizeofInstance=sizeof(struct GOSFS_Instance) + FIND_NUM_BYTES(superblock->size);
    Debug("size of instance %d bytes\n",sizeofInstance);
    rc = Release_FS_Buffer(gosfs_cache, p_buff);
    if (rc<0)
    {
        Print("Unable to release fs_buffer\n");
        rc = EFSGEN;
        goto finish;
    }
    p_buff = 0;
    
    // 分配足够的内存
    instance = Malloc(sizeofInstance);
    if (instance==0)
    {
        Print("Malloc failed to allocate memory\n");
        rc=ENOMEM;
        goto finish;
    }
    // 初始化mutex
    Mutex_Init(&instance->lock);
    instance->buffercache = gosfs_cache;
    bwritten = 0;
    superblock = &(instance->superblock);
    for (i=0; i<numBlocks; i++)
    {
        Print("fetching block[%ld]\n",i);
        rc = Get_FS_Buffer(gosfs_cache, i, &p_buff) ;

        if ((numBytes-bwritten)<GOSFS_FS_BLOCK_SIZE)
        {
            memcpy(((void*)superblock)+bwritten, p_buff->data, numBytes-bwritten);
            bwritten=bwritten+(numBytes-bwritten);
        }
        else
        {
            memcpy(((void*)superblock)+bwritten, p_buff->data, GOSFS_FS_BLOCK_SIZE);
            bwritten=bwritten+GOSFS_FS_BLOCK_SIZE;
        }
        
        rc = Release_FS_Buffer(gosfs_cache, p_buff);
        if (rc<0)
        {
            Print("Unable to release fs_buffer\n");
            rc = EFSGEN;
            goto finish;
        }
        p_buff = 0;
    }
    mountPoint->fsData = instance;
    rc = 0;
finish:
    return rc;
}

static struct Filesystem_Ops s_gosfsFilesystemOps = {
    &GOSFS_Format,
    &GOSFS_Mount,
};

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_GOSFS(void)
{
    //注册gosfs文件系统
    Register_Filesystem("gosfs", &s_gosfsFilesystemOps);
}

