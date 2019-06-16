#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int n;
    if (argc != 2 || (n = atoi(argv[1])) < 0) {
        fprintf(stderr, "usage: rot n\n");
        return(1);
    }
    int c;
    // Edits the value of n when it is greater then 26
    if (n > 26) {
	n %= 26;
    }
    while ((c = getchar()) != EOF) {
	// Doesn't change character if it is a non-character
	c += (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) ? n : 0;
	// Round robins c when it is greater then z or Z
	c -= !(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) ? 26 : 0;
	// Prints output
	printf("%c\n", (char) c);	
    }
    return(0);
}
