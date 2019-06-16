/*
 * fsh.c - the Feeble SHell.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "fsh.h"
#include "builtin.h"
#include "parse.h"
#include "error.h"

char ex[1000];
int showprompt = 1;
int laststatus = 0;  /* set by anything which runs a command */
extern void doCommand (struct parsed_line *p);
extern void exCommand (struct pipeline *pl);
extern char *isExecuteable (char *command);

int main()
{
    char buf[1000];
    struct parsed_line *p;
    extern void execute(struct parsed_line *p);

    while (1) {
        if (showprompt)
            printf("$ ");
        if (fgets(buf, sizeof buf, stdin) == NULL)
            break;
        if ((p = parse(buf))) {
            execute(p);
            freeparse(p);
        }
    }
    return(laststatus);
}

extern char **environ;
void execute(struct parsed_line *p)
{
    int custom = 0; // For a custom command
    int fd;
    for (;p ; p = p->next) {
		custom = 0; // Resets custom
		fflush(stdin); // Flushes stdin buffer
		fflush(stdout); // Flushes stdout buffer
		fflush(stderr); // Flushes stderr buffer
		switch (p->conntype) {
		    case CONN_SEQ: // ;
				break;
		    case CONN_AND: // &&
				p = (laststatus != 0) ? NULL: p;
				break;
		    case CONN_OR: // ||
				p = (laststatus == 0) ? NULL : p;
				break;
		}
		if (p == NULL) {
		    break; // Force exit if p is null
		}
		if (p->pl) {
		    if (strcmp(p->pl->argv[0], "exit") == 0) {
				laststatus = builtin_exit(p->pl->argv); // Exit
				custom = 1; // Custom command
		    } else if (strcmp(p->pl->argv[0], "cd") == 0) {
				laststatus = builtin_cd(p->pl->argv); // Changes directory
				custom = 1; // Custom command
		    }
		}
		if (custom == 0) { // Doesn't fork if there is a custom command
		switch (fork()) {
		    case -1:
				perror("fork");
				laststatus = 1;
				exit(1);
		    case 0: // fork() == 0
				if (p->inputfile) { // There is a <
				    if ((fd = open(p->inputfile, O_RDONLY)) < 0) {
						perror("readfile");
						laststatus = 1;
						exit(1);
				    } else {
				        dup2(fd, 0); // Sends stdin from file
				    }
				    close (fd); // Closes file descriptor
				}
				if (p->outputfile) { // There is a >
					if ((fd = open(p->outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
						perror("openfile");
						laststatus = 1;
						exit(1);
				    } else {
						dup2(fd, 1); // Sends stdout to file
				    }
					close(fd); // Closes file descriptor
				}
				if (p->pl->next) { // Pipes 2
				    doCommand(p); // Sends pipe
				}
				if (p->pl) { // Command
				    exCommand(p->pl); // Executes command
				}
				break;
		    default:
				wait(&laststatus); // Parent waits
				laststatus = WEXITSTATUS(laststatus);
		}
	}
    }
}
// Pipes commands
void doCommand(struct parsed_line *p) {
    int pipefd[2];
    if (pipe(pipefd)) {
		perror("pipe");
		laststatus = 1;
		exit(1);
    }
    
    switch(fork()) {
		case -1:
		    perror("pipefork");
		    laststatus = 1;
		    exit(1);
		case 0:
		    close(pipefd[0]); // Closes parent pipe
		    dup2(pipefd[1], 1); // Closes previous fd 1
		    close(pipefd[1]); // Cleanup
		    exCommand(p->pl); // Executes command
		default: 
		    close(pipefd[1]); // Closes child pipe
		    dup2(pipefd[0], 0); // Closes previous fd 0
		    close(pipefd[0]); // Cleanup
		    wait(&laststatus); // Wait
		    exCommand(p->pl->next); // Executes next command
    }
}
// Executes a command
void exCommand(struct pipeline *pl) {
	strcpy(ex, isExecuteable (pl->argv[0]));
	if (strcmp(ex, "\0") == 0) {
		fprintf(stderr, "%s: Command not found.\n", pl->argv[0]); // Invalid command
		laststatus = 1;
		exit(1);
	} else {
		laststatus = execve(ex, pl->argv, environ); // Executes command
	}
}
// Checks if a command is executeable or not
char * isExecuteable (char *command) {
    struct stat filestat;
    char *prefix[] = { "/bin", "/usr/bin", "." };
    int i;
    // When the command is /something or ./something
    if (strchr(command, '/')) {
		return (stat(command, &filestat) >= 0 && (filestat.st_mode & S_IXUSR) 
		    && strlen(command) < 1000) ? ex : "\0";
    } else {  // Otherwise it searches in /bin, /usr/bin and current dir
		for (i = 0; i < sizeof(prefix)/sizeof(*prefix); i++) { // Loops through prefixes
		    strcpy(ex, efilenamecons(prefix[i], command));
		    if (stat(ex, &filestat) >= 0 && (filestat.st_mode & S_IXUSR)) { // Gets command only if it is group executeable
				return ex;
		    }
		}
    }
    return "\0"; // Otherwise will return an error
}
