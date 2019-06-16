/*
 * parse.c - Feeble command parsing for the Feeble SHell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "parse.h"
#include "error.h"


#define MAXARGV 1000

enum token {
    identifier, directin, directout, doubledirectout,
    // Everything >= semicolon ends an individual "struct pipeline"
    semicolon, // ; New token
    ampersand, // & Executes in backgorund
    verticalbar, // | Pipes
    doubleampersand, // && Executes if and only if first command executes
    doublebar, // || Executes it first command doesn't execute
    doublepipe, // A double pipe
    eol // End of line
};

static enum token gettoken(char **s, char **argp); 
static char *ptok(enum token tok);


struct parsed_line *parse(char *s)
{
    struct parsed_line *retval;  // Remains freeparse()able at all times
    struct parsed_line *curline; // Current line input
    struct pipeline **plp;  // Where to append for '|' and '|||'
    char *argv[MAXARGV]; // Arguments
    enum token tok;
    int argc = 0; // Argument size
    int isdouble = 0; // Not used atm

    retval = curline = emalloc(sizeof(struct parsed_line)); // Allocate memory for line to be parse
    curline->conntype = CONN_SEQ;  /* i.e. always do this first command */
    curline->inputfile = curline->outputfile = NULL; // Sets inputfile and outputfile to null
    curline->output_is_double = 0; // Set output_is_double to 0
    curline->isbg = 0; // Set isbg to 0
    curline->pl = NULL; // Sets pl to 0
    curline->next = NULL; // Sets next to null
    plp = &(curline->pl); // Sets plp to null

    do {
        if (argc >= MAXARGV) // Gets error if there are too many arguments
            fatal("argv limit exceeded"); // Error message
        while ((tok = gettoken(&s, &argv[argc])) < semicolon) { // Loops while tokens exits
	    switch ((int)tok) {  /* Cast prevents stupid warning message about
				  * not handling all enum token values */
	    case identifier:
		argc++;  /* It's already in argv[argc];
		          * increment to represent a save */
		break;
	    case directin: // <
		if (curline->inputfile) { // Multiple inputs to direct
		    fprintf(stderr, // Error message
			    "syntax error: multiple input redirections\n");
		    freeparse(curline); // Frees current line
		    return(NULL);
		}
		if (gettoken(&s, &curline->inputfile) != identifier) { // Syntax error
		    fprintf(stderr, "syntax error in input redirection\n");
		    freeparse(curline); // Frees current line
		    return(NULL);
		}
		break;
	    case doubledirectout:
		curline->output_is_double = 1; // There is a double redirection
		/* Fall through */
	    case directout:
		if (curline->outputfile) { // Multiple outputs to direct to
		    fprintf(stderr,
			    "syntax error: multiple output redirections\n");
		    freeparse(curline); // Frees current line
		    return(NULL);
		}
		if (gettoken(&s, &curline->outputfile) != identifier) { // Syntax error
		    fprintf(stderr, "syntax error in output redirection\n");
		    freeparse(curline); // Frees current line
		    return(NULL);
		}
		break;
	    }
	}

	/* Cons up just-parsed pipeline component */
	if (argc) {
	    *plp = emalloc(sizeof(struct pipeline)); // Allocates memory to pipe
	    (*plp)->next = NULL; // Reset next token 
	    (*plp)->argv = eargvsave(argv, argc); // Adds null to end of argv
	    (*plp)->isdouble = isdouble; // Transfers double redirecton to pipeline
	    plp = &((*plp)->next); // Gets next pipe token
	    isdouble = 0; // Reset double
	    argc = 0; // Resets argc
	} else if (tok != eol) { // Empty command error
	    fprintf(stderr, "syntax error: null command before `%s'\n",
		    ptok(tok));
	    freeparse(curline); // Frees current line
	    return(NULL);
	}

	/* ampersanded? */
	if (tok == ampersand)
	    curline->isbg = 1; // Enable ampersand

	/* is this a funny kind of pipe (to the right)? */
	if (tok == doublepipe)
	    isdouble = 1; // Enable double pipe

	/* does this start a new struct parsed_line? */
	if (tok == semicolon || tok == ampersand || tok == doubleampersand || tok == doublebar) {
	    curline->next = emalloc(sizeof(struct parsed_line));
	    curline = curline->next;

	    curline->conntype =
		(tok == semicolon || tok == ampersand) ? CONN_SEQ
		: (tok == doubleampersand) ? CONN_AND
		: CONN_OR;
	    curline->inputfile = curline->outputfile = NULL; // No output for redirection
	    curline->output_is_double = 0; // Reset double
	    curline->isbg = 0; // Reset background process
	    curline->pl = NULL; // Reset pipeline
	    curline->next = NULL; // Reset next
	    plp = &(curline->pl); // Reassigns whole pipeline
	}

    } while (tok != eol); // Returns end of line
    return(retval);
}


/* (*s) is advanced as we scan; *argp is set iff retval == identifier */
static enum token gettoken(char **s, char **argp)
{
    char *p;

    while (**s && isascii(**s) && isspace(**s)) // Loops while there are arguments
        (*s)++; // Next token
    switch (**s) {
    case '\0': // End of file
        return(eol);
    case '<': // Input redirection
        (*s)++;
        return(directin);
    case '>': // Output redirection
        (*s)++; // Next token
	if (**s == '&') { // Runs command in the backgorund
	    (*s)++; // Next token
	    return(doubledirectout);
	}
        return(directout);
    case ';': // New command
        (*s)++; // Next token
        return(semicolon);
    case '|': // Pipe
	if ((*s)[1] == '|') { // Double vertical pipe
	    *s += 2; //Gets argument
	    return(doublebar);
	}
        (*s)++; // Next token
	if (**s == '&') { // Runs command in background
	    (*s)++; // Next token
	    return(doublepipe);
	}
        return(verticalbar);
    case '&': // Run in background
	if ((*s)[1] == '&') { // Double ampersand
	    *s += 2; // Gets command
	    return(doubleampersand);
	} else { // Single ampersand
	    (*s)++; // Next token
	    return(ampersand);
	}
    /* else identifier */
    }

    /* it's an identifier */
    /* find the beginning and end of the identifier */
    p = *s;
    while (**s && isascii(**s) && !isspace(**s) && !strchr("<>;&|", **s))
        (*s)++; // Next token
    *argp = estrsavelen(p, *s - p); // Gets length of token
    return(identifier);
}


static char *ptok(enum token tok)
{
    switch (tok) {
    case directin:
	return("<");
    case directout:
	return(">");
    case semicolon:
	return(";");
    case verticalbar:
	return("|");
    case ampersand:
	return("&");
    case doubleampersand:
	return("&&");
    case doublebar:
	return("||");
    case doubledirectout:
	return(">&");
    case doublepipe:
	return("|&");
    case eol:
	return("end of line");
    default:
	return(NULL);
    }
}


static void freepipeline(struct pipeline *pl)
{
    if (pl) {
	char **p;
        for (p = pl->argv; *p; p++)
            free(*p); // Frees each pipeline
        free(pl->argv); // Frees argument size
        freepipeline(pl->next); // Frees next token
        free(pl); // Frees pipeline
    }
}


void freeparse(struct parsed_line *p)
{
    if (p) { // Frees next token
	freeparse(p->next);
	if (p->inputfile) // Frees inputfile memory
	    free(p->inputfile);
	if (p->outputfile) // Frees outputfile memory
	    free(p->outputfile);
	freepipeline(p->pl); // Frees pipeline
	free(p); // Frees parseline
    }
}
