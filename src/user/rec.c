/*
 * A user mode program which uses deep recursion
 * to test stack growth.
 */

#include <conio.h>
#include <string.h>
#include <sched.h>

/**
 * a STUFF_LENGTH value of 1024 reserves a complete page on the stack
 */

#define STUFF_LENGTH	1024

int i;
int r = 0;
int e = 0;

/* change recurse to 5-10 to see stack faults without page outs */
int depth = 512;


void Recurse(int x)
{
    int stuff[STUFF_LENGTH];
    r++;

    if (x == 0) return;

    for (i=0; i<STUFF_LENGTH; i++)
       stuff[i] = x;

    if (x % 50 == 1)
       Print("\ncalling Recurse %4d ", x);
    else
       Print(".");
    Recurse(x-1);

    if (e > 10) return;

    for (i=0; i<STUFF_LENGTH; i++) {
       if (stuff[i] != x)  e++;
    }

    r--;
}

int main(int argc, char **argv)
{
    if (argc > 1) {
	depth = atoi(argv[1]);
	Print("Depth is %d\n", depth);
    }

    int time = Get_Time_Of_Day();
    if (depth % 50 != 1)
        Print("                     ");

    Recurse(depth);
    Print("\nTime: %d ticks\n", Get_Time_Of_Day() - time);

    return 0;
}

