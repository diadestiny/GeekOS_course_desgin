/*
 * Access Control Lists
 *
 */

#ifndef ACL_H
#define ACL_H

#include <geekos/ktypes.h>

int SetAcl(const char *name, int uid, int permissions);
int SetSetUid(const char *name, int setUid);
int SetEffectiveUid(int uid);
int GetUid();

#endif  /* ACL_H */

