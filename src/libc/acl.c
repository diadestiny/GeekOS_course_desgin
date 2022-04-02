/*
 * Access Control Lists
 * Copyright (c) 2007, Clemens Krainer <ClemensDaniel.Krainer@sbg.ac.at>
 * Copyright (c) 2007, Bernhard Danninger <bid-soft@gmx.at>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <string.h>
#include <acl.h>

DEF_SYSCALL(SetAcl,SYS_SET_ACL,int, (const char* name, int uid, int permissions),
    const char *arg0 = name; size_t arg1 = strlen(name); int arg2 = uid; int arg3 = permissions;,
    SYSCALL_REGS_4)
DEF_SYSCALL(SetSetUid,SYS_SET_SET_UID,int, (const char* name, int setUid),
    const char *arg0 = name; size_t arg1 = strlen(name); int arg2 = setUid;,
    SYSCALL_REGS_3)
DEF_SYSCALL(SetEffectiveUid,SYS_SET_EFFECTIVE_UID,int,(int uid),int arg0 = uid;,SYSCALL_REGS_1)
DEF_SYSCALL(GetUid,SYS_GET_UID,int,(void),,SYSCALL_REGS_0)

