#include <stdio.h>
#include <stdlib.h>

int main ( int argc, char **args ) {
    int n; int state = 0; int i = 1; int c;
    if (argc != 2 || (n = atoi(args[1])) < 0) {
		fprintf(stderr, "usage: nth n\n");
		return(1);
    }
    while ((c = getchar()) != EOF) {
		if (state == 0) {
		    if (c == '\t' || c == ' ') {		   
				state = 0;
		    } else if (c == '\n') {
				state = 0; i = 1; printf("\n");
		    } else if (i < n) {
				state = 1;
		    } else {
				state = 2; printf("%c", (char) c);
		    }
		} else if (state == 1) {
		    if (c == '\t' || c == ' ') {		   
				state = 0; i++;
		    } else if (c == '\n') {
				i = 1; state = 0; printf("\n");
		    } else {
				state = 1;
		    }
		} else if (state == 2) {
		    if (c == '\t' || c == ' ') {
				state = 3;
		    } else if (c == '\n') {
				i = 1; state = 0; printf("\n");
		    } else {
				state = 2; printf("%c", (char) c);
		    }
		} else {
		    if (c == '\n') {
				i = 1; state = 0; printf("\n");
		    } else {
				state = 3;
		    }
		}
    }
    return (0);
}
