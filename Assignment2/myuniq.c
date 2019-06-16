#include <stdio.h>
#include <unistd.h>
#include <string.h>

int counter = 1; // Counts occurences of a unique string
char holder [500] = "";
char phrase [500] = "";
int exitStatus = 0;
int main(int argc, char **argv) {
    int c = 0;
    int errFlag = 0; // Sets if an error is encountered in syntax
    int cFlag = 0; // Stores if the -c option is set
    int cCounter = 0; // Stores amount of options inputted
    // Checks for optional arguments
    while ((c = getopt(argc, argv, "c")) != -1) {
		switch (c) {
		    case 'c': // Optional counter is on
				cFlag = 1; // -c has been inputted
				cCounter++; // Increment optional counter
				break;
		    case '?': // Optional characters that aren't accepted 
				errFlag = 1; // usage message will output
				break;
		}
    }
    int n;
    int forceQuit = 0; // Forces program to terminate if file is more then 500 characters
    // Checks if any of the arguments exceed 500 characters
    for (n = 1; n < argc; n++) {
		if (strlen(argv[n]) > 500) {
		    printf("%s: File name too long\n", argv[n]);
		    forceQuit = 1;
		}
    }

    if (forceQuit == 1) {
		return 1;
    }
    // Outputs the error if an invalid option is set
    if (errFlag == 1) {
		fprintf(stderr, "usage: ./a.out [-c] [file ...]\n");
		return 1;
    }
    extern void processR (int cFlag);
    extern void process (int argc, char **argv, int cFlag);
    if (argc - 2 < cCounter) {
		processR(cFlag); // Process as standard input to keep looping
    } else {
		process (argc, argv, cFlag); // Process as arguments
    }
    return exitStatus;
}

void processR (int cFlag) { // Repeatedly asks for user input 
    while (fgets(phrase, 500, stdin) != NULL) { // Scans for strings and caps at 500 characters
		if (strlen(holder) == 0) { // Copies the input to holder if it is blank
		    strcpy(holder, phrase); 
		    counter = 1; // Resets counter to 1
		} else if (strcmp(holder, phrase) != 0) { // Outputs previous input if it changes
		    (cFlag == 1) ? printf("%d %s", counter, holder) : printf("%s", holder); // Output
		    strcpy(holder, phrase); // Copies new phrase to holder
		    counter = 1; // Resets counter to 1
		} else {
		    counter++; // Increments counter
		}
    }
}

void process (int argc, char **argv, int cFlag) {   
    int clearFlag = 0; // Indicates the prescence of a "--" in the input
    int i; // Looper for the arguments
    FILE *outfile; // The file in question to be extracted for content
    char contents[argc][500]; // Holds contents of files/input
    int size = 0; // Size of the array
    for (i = 1; i < argc; i++) {
		outfile = fopen(argv[i], "r"); // Opens file
		if (strcmp(argv[i], "-c") != 0 || clearFlag == 1) { // Treats optional arguments unitl a "--" is present
		    if (strcmp(argv[i], "-") == 0) {  //Inserts '-' into contents
				strcpy(contents[size], "-");
				size++; // Increments size of array
		    } else if (outfile == NULL) {
				printf("%s: No such file or directory\n", argv[i]); // Error message
				exitStatus = 1;
		    } else {
				while(fgets(contents[size], 500, outfile) != NULL) { // Copies file contents to array while content exists
				    size++; // Increments size of array
				}
		    }
		}
		clearFlag = (strcmp(argv[i], "--") == 0) ? 1 : clearFlag; // Checks for a "--" termination option
    }
    int pos;
    for (pos = 1; pos < size; pos++) { // Loops through contents of file
		if (strcmp(contents[pos], "-") == 0) { //If the input is -, will go into standard input mode
		    strcpy(holder, contents[pos-1]); // Save the previous string
		    processR(cFlag);
		} else if (strcmp(contents[pos - 1], contents[pos]) == 0) {
		    counter++; // Increment counter
		} else {
		    (cFlag == 1) ? printf("%d %s", counter, contents[pos-1]) : printf("%s", contents[pos-1]); // Outputs uniqueness of string
		    counter = 1;
		}
    }
    (cFlag == 1) ? printf("%d %s", counter, contents[pos-1]) : printf("%s", contents[pos-1]); // Outputs final output
}
