/*
 * mkdir - Create a directory
 * Copyright (c) 2006 Danninger, Krainer
 * $Revision: 1.0 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>

int main(int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        Print("Usage: rm <file or directory>\n");
	Exit(1);
    }

    rc = Delete(argv[1]);
    if (rc != 0)
	Print("remove directory: %s\n", Get_Error_String(rc));

    return !(rc == 0);
}
