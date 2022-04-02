
/*
 * A test program for GeekOS user mode
 */

#include <conio.h>
#include <process.h>
#include <fileio.h>


int main(void)
{
    int rc;
    int sink, source;
    int rd, wr;

    rd = wr = -1;

    rc = Create_Pipe(&rd, &wr);
//  Print("Create_Pipe returned rc=%d, rd=%d, wr=%d\n", rc, rd, wr);

    source = Spawn_Program ("/c/hello.exe", "/c/hello.exe", 0, wr);
    sink = Spawn_Program ("/c/cat.exe", "/c/cat.exe", rd, 1);
    Close(wr);
    Close(rd);

    Wait(source);
    Wait(sink);
    return 0;
}
