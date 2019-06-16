#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>

static int G = 2048000;
static int M = 2028;
static int K = 2;
int main(int argc, char **argv) {
    int c;
    int exitStatus = 0;
    int hflag = 0; // Status for h option
    int sflag = 0; // Status for s option
    int errflag = 0; // Status for invalid options
    // Searches for options
    while ((c = getopt(argc, argv, "hs")) != -1) {
		switch (c) {
		    case 'h': // Option -h
				hflag = 1; // Sets hflag to on
				break;
		    case 's': // Option -s
				sflag = 1; // Sets sflag to on
				break;
		    case '?': // Invalid option
				errflag = 1; // Sets errflag to on
				break;
		}
    }
    int nonOpt = 0; // Counts amount of non-options
    int i;
    int forceQuit = 0; // Status to force quit if char length is longer then 2000
    // Error checks if all arguments are options and argument length
    for (i = optind; i < argc; i++) {
		if (strlen(argv[i]) > 2000) { // If argument is greater then 2000 characters
		    printf("%s: File name too long", argv[i]); // Error message of too long argument
		    forceQuit = 1; // Sets force quit status to on
		}
		nonOpt++;
    }
    // Forces the program to close
    if (forceQuit == 1) {
		return 1;
    }
    // Outputs an error message if invalid or 
    if (errflag == 1 || nonOpt == 0) {
		fprintf(stderr, "usage: ./mydu [-sh] dir ...\n");
		return 1;
    }
    DIR *dp;
    int total = 0;
    extern void outmod(long size, int hflag);
    extern long process(char* path, int sflag, int hflag);
    char out[2000];
    // Processes arguments
    for (i = optind; i < argc; i++) {
		if ((dp = opendir(argv[i])) == NULL) { // Directory doesn't exist
		    printf("%s: No such file or directory\n", argv[i]);
		    exitStatus = 1;
		} else { // Processes directory if it exists
		    strcpy(out, argv[i]);
		    total = process(out, sflag, hflag);
		    outmod(total, hflag);
		    printf("%s\n", argv[i]);
		}
    }
    return exitStatus;  
}

long process(char* path, int sflag, int hflag) { 
    extern void outmod (long size, int hflag);
    char newfile[2000];
    long size = 0;
    DIR *dp;
    struct dirent *p;
    struct stat fileStat;
    if ((dp = opendir(path)) != NULL) { // Processes file if it is a directory
		if (path[strlen(path)-1] != '/') {
		    strcat(path, "/"); // Appends / to the end
		}
		while ((p = readdir(dp)) != NULL) { // Loops through ls
		    // Ignores . and .. to prevent infinite recursion
		    if ((strcmp(p->d_name, ".") != 0) && (strcmp(p->d_name, "..") != 0)) {
				strcpy(newfile, path); // Copies path
				strcat(newfile, p->d_name); // Appends path
				size+=process(newfile, sflag, hflag); // Adds size
		    } 
		}
    } else { // Otherwise check if its a file or not
		if (lstat(path, &fileStat) < 0) {
		    return 0; // If it isn't a file return 0
		} else {
		    if (sflag == 0) {
				outmod(fileStat.st_blocks/2, hflag); // Prints, blocks, G, M, K
				printf("%s\n", path);
		    }
		    return (fileStat.st_blocks/2); // Otherwise it will return the blocks
		}
    }
    closedir(dp); // Closes the directory
    return size; // Returns the size
}

// Prints size data and/or converts to their relative sizes if h is on
void outmod(long size, int hflag) {
    char *prefix = "GMK";
    if (hflag == 1) {
		if (size > G) { // Size is at least a gig in size
		    size/=G; // Divides by a gig
		    printf("%ld%c ", size, prefix[0]); // Prints size
		} else if (size > M) { // Size is at least a meg in size
		    size = (size + 1048) / M;
		    printf("%ld%c ", size, prefix[1]); // Prints size
		} else { // Size is at least a kilo in size
		    size/=K; // Divides by a kilo
		    printf("%ld%c ", size, prefix[2]); // Prints size
		}
    } else {
		printf("%ld ", size); // Prints size
    }
}
