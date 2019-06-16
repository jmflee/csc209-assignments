#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

int main (int argc, char **argv) {
    // Returns an error message or the usage of the command
    if (argc < 2) {
		fprintf(stderr, "usage: mywhich commandname ...\n");
		return 1;
    }
    int exitStatus = 0; // Status if there is an error or not
    int x, y;
    const char *pre[] = { "", "/bin/", "/usr/bin/" }; // Predefined folders to look for
    char concat[1000]; // Location is at max 1000 characters
    int notExists;
    // Iterate through arguments
    for (x = 1; x < argc; x++) {
        struct stat fileStat;
		notExists = 0; // Resets file counter
		for (y = 0; y < 3; y++) { // Loops throught file prefixes
		    strcpy(concat, pre[y]); // Copies the predefined folder locations to concat
		    strcat(concat, argv[x]); // Concactenaties folder locations with argument
		    if ((stat(concat, &fileStat) < 0) || !(fileStat.st_mode & S_IXUSR)) {
				notExists++; // Increment parts where file isn't found
		    } else {
				printf("%s\n", concat); // Outputs place where file is found
		    }
		}
		if (notExists ==  3) {
		    exitStatus = 1; // Command not found
		    printf("%s: Command not found.\n", argv[x]); // Outputs error message if file wasn't found
		}
    }
    return exitStatus;
}
