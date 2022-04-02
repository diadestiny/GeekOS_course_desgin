#include <conio.h>
#include <fileio.h>
#include <process.h>
#include <string.h>
#include "libuser.h"
#include "process.h"

static void Print_Error(const char *msg, int fd)
{
    Print("%s: %s\n", msg, Get_Error_String(fd));
    Exit(1);
}

#define MESSAGE "Hello, world. this is a hdd test\n"

int main(int argc, char **argv)
{
    int fd, rc, i, start, elapsed, scr_sem, hdd_sem, count=0;
    char *filename;
	int runtime=200;
	
	start = Get_Time_Of_Day();
	scr_sem = Create_Semaphore ("screen" , 1) ;
	hdd_sem = Create_Semaphore ("hdd" , 1) ;
	
    if (argc < 2) {
	Print("Usage: %s <filename>\n", argv[0]);
	Exit(1);
    }
    filename = argv[1];
	
	if (argc==3)
  	{
	  runtime=atoi(argv[2]);
  	}

	
	
	while(Get_Time_Of_Day()<start+runtime)
	{
    fd = Open(filename, O_CREATE|O_WRITE);
    if (fd < 0)
	Print_Error("Could not open file", fd);
	for (i=0; i<10; i++)
	{
		//P (hdd_sem) ;
    	rc = Write(fd, MESSAGE, strlen(MESSAGE));
    	if (rc < 0)
		{
			Print_Error("Could not write to file", rc);
			break;
		}
		//V (hdd_sem) ;
		if (count%100==0) 
		{
			Set_Attr(ATTRIB(BLACK, RED|BRIGHT));
			Print("H"); 
			Set_Attr(ATTRIB(BLACK, GRAY));
		}
		count++;
	}
	rc = Close(fd);
    if (rc < 0)
	Print_Error("Could not close file", rc);
	}

    
	
	elapsed = Get_Time_Of_Day() - start;
    P (scr_sem) ;
    //Print("Process HDD is done at time: %d\n", elapsed) ;
	Print("\nHDD: number of runs: %d",count);
    V(scr_sem);
    

    return 0;
}
