/*
 * A acl test program for GeekOS user mode
 */

#include "libuser.h"
#include <string.h>

int main(int argc , char ** argv)
{
    int ret;
    int su = 0;

    if (argc != 3) {
        Print("Usage: setuid <file> set | clear\n");
	return 1;
    }

    if (!strcmp(argv[2], "set")) {
        su = 1;
    } else if (!strcmp(argv[2], "clear")) {
        su = 0;
    } else {
        Print("Usage: setuid <file> set | clear\n");
	return 1;
    }

    ret = SetSetUid(argv[1], su);
    if (ret) {
        Print("SetAcl returned %d, %s\n", ret, Get_Error_String(ret));
    }

    return ret;
}
