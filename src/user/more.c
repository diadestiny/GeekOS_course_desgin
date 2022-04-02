
/*
 * more - displays the contents of files one screen at a time.
 * Copyright (c) 2007, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.12 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <limits.h>
#include <conio.h>
#include <process.h>
#include <fileio.h>
#include <string.h>

#define PAGE_LENGTH     24


int read_line (int inFd, void* buf, int maxChars) {
    int rc;
    char *ptr = buf;
    size_t n = 0;
    char c;
    bool done = false;
    
    do {
        rc = Read(inFd, &c, 1);
        if (rc <= 0) 
            break;

        if (c == '\n' || c == '\r')
            done = true;

        if (n < maxChars) {
            *ptr++ = c;
            ++n;
        }
    }
    while (!done);

    *ptr = '\0';
    return n;
}



int More (int inFd, ulong_t size)
{
    int rc, ret;
    int read;
    char buffer[1024];
    ulong_t line = 0;
    ulong_t chars = 0;
    ulong_t next = line + PAGE_LENGTH;
    int col = 0, row = 0;

    for (read =0; read < size; read += ret) {

        ret = read_line (inFd, buffer, sizeof(buffer));
        if (ret < 0) {
            Print("error reading file for copy: %s\n", Get_Error_String(ret));
            return 1;
        }
        if (!ret)
            break;

        chars += ret;
        buffer[ret] = '\0';
        if (buffer[ret-1] == '\n')
            ++line;

        rc = Write(1, buffer, ret);
        if (rc < 0) {
            Print("Could not write to stdout: %s\n", Get_Error_String(rc));
            return 1;
        }
//Print ("More: line=%ld next=%ld\n", line, next);

        if (line >= next) {
            bool ok = true;
            char msg[100];
            Get_Cursor (&row, &col);
            do {
                if (size != ~0L)
                    snprintf (msg, sizeof(msg),"--Mehr--(%lu/%lu) ", chars,size);
                else
                    snprintf (msg, sizeof(msg),"--Mehr--(%lu) ", line);
                Write(1, msg, strlen(msg));
                ok = true;
                Keycode k = Cons_Key();
                switch (k) {
                   case '\r':
                   case '\n':   next = line + 1; break;
                   case 16483:  /* ctrl c */
                   case 4177: case 'Q':
                   case 'q':    Put_Cursor (row, col); return 0; break;
                   case ' ':    next = line + PAGE_LENGTH; break;
                   default:     ok = false; break;
                }
                Put_Cursor (row, col);
            } while (!ok);
        }
    }

    return 0;
}



int main(int argc, char *argv[])
{
    int i;
    int ret;
    int inFd;
    struct VFS_File_Stat stat;

    if (argc == 1)
        return More (STDIN_FILENO, ~0L);

    for (i = 1; i < argc; ++i) {
        const char *filename = argv[i];

        Print ("opening file %s\n", filename);
        inFd = Open(filename, O_READ);
        if (inFd < 0) {
            Print ("Error: unable to open %s\n", filename);
            return 1;
        }

        ret = FStat(inFd, &stat);
        if (ret != 0) {
            Print ("Error: could not stat %s: %s\n", filename, Get_Error_String(ret));
            return 1;
        }

        if (stat.isDirectory) {
            Print ("can not view directories\n");
            return 1;
        }

Print ("More: fd=%d size=%d\n", inFd, stat.size);
        ret = More (inFd, stat.size);

        Close(inFd);

        if (!ret)
            return ret;
    }

    return 0;
}

