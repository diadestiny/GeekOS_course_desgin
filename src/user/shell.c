/*
 * A really, really simple shell program
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.18 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <fileio.h>
#include <conio.h>
#include <process.h>
#include <string.h>
#include <kernel.h>
#include <acl.h>

#define BUFSIZE 79
#define DEFAULT_PATH "/c:/d:/a"

#define INFILE	0x1
#define OUTFILE	0x2
#define PIPE	0x4

#define ISSPACE(c) ((c) == ' ' || (c) == '\t')

struct Process {
    int flags;
    char program[BUFSIZE+1];
    char infile[BUFSIZE+1];
    char outfile[BUFSIZE+1];
    char *command;
    int pid;
    int readfd, writefd;
    int pipefd;
};

char *Strip_Leading_Whitespace(char *s);
void Trim_Newline(char *s);
char *Copy_Token(char *token, char *s);
int Build_Pipeline(char *command, struct Process procList[]);
void Spawn_Single_Command(struct Process procList[], int nproc, const char *path);
void Spawn_Pipeline(struct Process procList[], int nproc, const char *path);

/* Maximum number of processes allowed in a pipeline. */
#define MAXPROC 5

int exitCodes = 0;

int main(int argc, char **argv)
{
    int nproc;
    char commandBuf[BUFSIZE+1];
    struct Process procList[MAXPROC];
    char path[BUFSIZE+1] = DEFAULT_PATH;
    char *command;

    /* Set attribute to gray on black. */
    Print("\x1B[37m");

    while (true) {
	/* Print shell prompt (bright cyan on black background) */
	Print("\x1B[1;36m$\x1B[37m ");

	/* Read a line of input */
	Read_Line(commandBuf, sizeof(commandBuf));
	command = Strip_Leading_Whitespace(commandBuf);
	Trim_Newline(command);

	/*
	 * Handle some special commands
	 */
	if (strcmp(command, "exit") == 0) {
	    /* Exit the shell */
	    break;
	} else if (strcmp(command, "pid") == 0) {
	    /* Print the pid of this process */
	    Print("%d\n", Get_PID());
	    continue;
        } else if (strcmp(command, "ps") == 0) {
            /* print processlist to screen */
            Print_ProcessList();
            continue;
        } else if (strncmp(command, "info", 4) == 0) {
            int uid = GetUid();
            /* print system information to screen */
            Print_System_Info(SYS_INFO_PAGING|SYS_INFO_SCHEDULER);
            Print ("User id=%d\n", uid);
            continue;
        } else if (strcmp(command, "paging-default") == 0) {
            Select_Paging_Algorithm(PAGING_DEFAULT);
            continue;
        } else if (strcmp(command, "paging-wsclock") == 0) {
            Select_Paging_Algorithm(PAGING_WSCLOCK);
            continue;
	} else if (strcmp(command, "exitCodes") == 0) {
	    /* Print exit codes of spawned processes. */
	    exitCodes = 1;
	    continue;
	} else if (strcmp(command, "path") == 0) {
            Print ("path=%s\n",path);
	    continue;
	} else if (strncmp(command, "path=", 5) == 0) {
	    /* Set the executable search path */
	    strcpy(path, command + 5);
	    continue;
        } else if (strcmp(command, "help") == 0) {
            Print_String("Shell Commands:\n"
               "   exit ............. exit this shell\n"
               "   pid .............. print the process id of this shell\n"
               "   ps ............... show process information\n"
               "   info ............. show system information\n"
               "   paging-default ... set default paging algorithm\n"
               "   paging-wsclock ... set WS Clock paging algorithm\n"
               "   exitCodes ........ print exit codes of spawned processes\n"
               "   path='...' ....... set the path environment variable\n"
               "   help ............. this help information\n"
            );
            continue;
	} else if (strcmp(command, "") == 0) {
	    /* Blank line. */
	    continue;
	}

	/*
	 * Parse the command string and build array of
	 * Process structs representing a pipeline of commands.
	 */
	nproc = Build_Pipeline(command, procList);
	if (nproc <= 0)
	    continue;

	Spawn_Pipeline(procList, nproc, path);
    }

    Print_String("DONE!\n");
    return 0;
}

/*
 * Skip leading whitespace characters in given string.
 * Returns pointer to first non-whitespace character in the string,
 * which may be the end of the string.
 */
char *Strip_Leading_Whitespace(char *s)
{
    while (ISSPACE(*s))
	++s;
    return s;
}

/*
 * Destructively trim newline from string
 * by changing it to a nul character.
 */
void Trim_Newline(char *s)
{
    char *c = strchr(s, '\n');
    if (c != 0)
	*c = '\0';
}

/*
 * Copy a single token from given string.
 * If a token is found, returns pointer to the
 * position in the string immediately past the token:
 * i.e., where parsing for the next token can begin.
 * If no token is found, returns null.
 */
char *Copy_Token(char *token, char *s)
{
    char *t = token;

    while (ISSPACE(*s))
	++s;
    while (*s != '\0' && !ISSPACE(*s))
	*t++ = *s++;
    *t = '\0';

    return *token != '\0' ? s : 0;
}

/*
 * Build process pipeline.
 */
int Build_Pipeline(char *command, struct Process procList[])
{
    int nproc = 0, i;

    while (nproc < MAXPROC) {
        struct Process *proc = &procList[nproc];
        char *p, *s;

        proc->flags = 0;

        command = Strip_Leading_Whitespace(command);
        p = command;

        if (strcmp(p, "") == 0)
	    break;

        ++nproc;

        s = strpbrk(p, "<>|");

        /* Input redirection from file? */
        if (s != 0 && *s == '<') {
	    proc->flags |= INFILE;
	    *s = '\0';
	    p = s+1;
	    s = Copy_Token(proc->infile, p);
	    if (s == 0) {
	        Print("Error: invalid input redirection\n");
	        return -1;
	    }
	    p = s;

	    /* Output redirection still allowed for this command. */
	    p = Strip_Leading_Whitespace(p);
	    s = (*p == '>' || *p == '|') ? p : 0;
        }

        /* Output redirection to file or pipe? */
        if (s != 0 && (*s == '>' || *s == '|')) {
	    bool outfile = (*s == '>');
	    proc->flags |= (outfile ? OUTFILE : PIPE);
	    *s = '\0';
	    p = s+1;
	    if (outfile) {
	        s = Copy_Token(proc->outfile, p);
	        if (s == 0) {
		    Print("Error: invalid output redirection\n");
		    return -1;
	        }
	        p = s;
	    }
        }

        proc->command = command;
        /*Print("command=%s\n", command);*/
        if (!Copy_Token(proc->program, command)) {
	    Print("Error: invalid command\n");
	    return -1;
        }

        if (p == command)
	    command = "";
        else
	    command = p;
    }

    if (strcmp(command,"") != 0) {
	Print("Error: too many commands in pipeline\n");
	return -1;
    }


    /*
     * Check commands for validity
     */
    for (i = 0; i < nproc; ++i) {
	struct Process *proc = &procList[i];
	if (i > 0 && (proc->flags & INFILE)) {
	    Print("Error: input redirection only allowed for first command\n");
	    return -1;
	}
	if (i < nproc-1 && (proc->flags & OUTFILE)) {
	    Print("Error: output redirection only allowed for last command\n");
	    return -1;
	}
	if (i == nproc-1 && (proc->flags & PIPE)) {
	    Print("Error: unterminated pipeline\n");
	    return -1;
	}
    }

    return nproc;
}

/*
 * Spawn a single command.
 */
void Spawn_Single_Command(struct Process procList[], int nproc, const char *path)
{
    int pid;

    if (nproc > 1) {
	Print("Error: pipes not supported yet\n");
	return;
    }
    if (procList[0].flags & (INFILE|OUTFILE)) {
	Print("Error: I/O redirection not supported yet\n");
	return;
    }

    pid = Spawn_With_Path(procList[0].program, procList[0].command,
	0, 1,
	path);
    if (pid < 0)
	Print("Could not spawn process: %s\n", Get_Error_String(pid));
    else {
	int exitCode = Wait(pid);
	if (exitCodes)
	    Print("Exit code was %d\n", exitCode);
    }
}

/*
 * Spawn a pipeline.
 */
void Spawn_Pipeline(struct Process procList[], int nproc, const char *path)
{
    int i;
    int numSpawned, exitCode=0;
    bool err = false;
    struct Process *prev;

    /*
     * Spawn all processes in the pipeline, opening input/output files
     * and pipes as needed.
     */
    for (i = 0, numSpawned = 0, prev = 0; !err && i < nproc; ++i, ++numSpawned) {
	struct Process *proc = &procList[i];
	int readfd = 0, writefd = 1;

	proc->readfd = proc->writefd = proc->pipefd = proc->pid = -1;

	/* Open input file */
	if (proc->flags & INFILE) {
	    readfd = Open(proc->infile, O_READ);
	    if (readfd < 0) {
		Print("Could not open file: %s\n", Get_Error_String(readfd));
		err = true;
	    }
	}

	if (!err) {
	    /* Open output file or pipe */
	    if (proc->flags & OUTFILE) {
		writefd = Open(proc->outfile, O_WRITE|O_CREATE);
		if (writefd < 0) {
		    Print("Could not open file: %s\n", Get_Error_String(writefd));
		    err = true;
		}
	    } else if (proc->flags & PIPE) {
		int rc = Create_Pipe(&proc->pipefd, &writefd);
		if (rc != 0) {
		    Print("Could not create pipe: %s\n", Get_Error_String(rc));
		    err = true;
		}
	    }
	}

	/* Is output being piped from previous process? */
	if (!err && prev != 0 && (prev->flags & PIPE)) {
	    readfd = prev->pipefd;
	}

	proc->readfd = readfd;
	proc->writefd = writefd;

	if (!err) {
	    proc->pid = Spawn_With_Path(proc->program, proc->command, proc->readfd, proc->writefd, path);
	    if (proc->pid < 0) {
		Print("Could not spawn process: %s\n", Get_Error_String(proc->pid));
		err = true;
	    }
	}

	prev = proc;
    }

    /*
     * Close all pipes and I/O redirection file descriptors
     * (except the shell's stdin and stdout).
     * This won't affect the child processes because they have
     * their own Cloned versions of the File objects.
     */
    for (i = 0, prev = 0; i < numSpawned; ++i) {
	struct Process *proc = &procList[i];

	if (proc->readfd > 1)
	    Close(proc->readfd);
	if (proc->writefd > 1)
	    Close(proc->writefd);

	if (i == numSpawned - 1 && (proc->flags & PIPE)) {
	    /*
	     * Process meant to read from this pipe was never spawned.
	     * The read fd for the pipe is stored in the "pipefd" field.
	     */
	    Close(proc->pipefd);
	}
    }

    /*
     * Wait for all processes to exit.
     */
    for (i = 0; i < numSpawned; ++i) {
	struct Process *proc = &procList[i];

	if (proc->pid >= 0)
	    exitCode = Wait(proc->pid);
    }

    if (numSpawned > 0 && exitCodes)
	Print("Exit code was %d\n", exitCode);
}

