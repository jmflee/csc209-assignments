#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "parse.h"
#include "util.h"
#include "chatsvr.h"

struct client {
    int fd;
    char buf[MAXMESSAGE+16];  /* partial line(s) */
    int bytes_in_buf;  /* how many data bytes in buf (after nextpos) */
    char *nextpos;  /* if non-NULL, move this down to buf[0] before reading */
    char name[MAXHANDLE + 1]; /* name[0]==0 means no name yet */
    struct client *next;
} *top = NULL;

static char *myreadline(struct client *m);
static void commit(struct client *m);
static void receiveAndProcess(struct client *m);

int main(int argc, char **argv)
{
    static char marvin[] = "Marvin\r\n";
    struct sockaddr_in r;
    struct hostent *hp;
	struct client *m = malloc(sizeof(struct client)); // Reserve memory for client
    fd_set fdlist;
    // Gets port # otherwise will default to 1234
    int port = (argc == 3) ? atoi(argv[2]) : 1234;
    // No hostname
    if (argc <= 1 || argc > 3) { 
        fprintf(stderr, "usage: %s host [port]\n", argv[0]);
        return(1);
    } 
    // Invalid port #
    if (port == 0) { 
        fprintf(stderr, "%s: port number must be a positive integer\n", argv[0]);
        return(1);
    }
    // Host does not exit
    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "%s: no such host\n", argv[1]);
        return(1);
    }
    // Bad host address
    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) { 
        fprintf(stderr, "%s: not an internet protocol host name\n", argv[2]);
        return(1);
    }
    // Gets file descriptor of socket
    if ((m->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return(1);
    }
    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    memcpy(&r.sin_addr, hp->h_addr_list[0], hp->h_length);
    r.sin_port = htons(port); // Gets port

    // Connects to address
    if (connect(m->fd, (struct sockaddr *)&r, sizeof r) < 0) {
        perror("connect()");
        return(1);
    }
	
    m->bytes_in_buf = 0; // bytes in buf default to 0
    m->nextpos = NULL; // No nextpos by default
    m->next = NULL; // No next by default

	while(!myreadline(m)) {} // Reads server banner

    // Writes Marvin as handle to server
    if (write(m->fd, marvin, sizeof marvin - 1) != sizeof marvin - 1) {
        perror("write()");
        return(1);
    }
    bzero(m->buf, MAXMESSAGE + 16);
    while (1) {
        fflush(stdout);  // Flushes stdout buffer
        fflush(stdin); // Flushes stdin buffer
        FD_ZERO(&fdlist); // Sets fdlist to zero
        FD_SET(m->fd, &fdlist); // Sets fd
        FD_SET(STDIN_FILENO, &fdlist); // Sets stdin
        switch (select(m->fd + 1, &fdlist, NULL, NULL, NULL)) {
            case -1: // Select fails
                perror("select()");
                break;
            default: // Select works
                if (FD_ISSET(m->fd, &fdlist)) { // When the server sends data
                    receiveAndProcess(m); // Processes data from server
                } else if (FD_ISSET(STDIN_FILENO, &fdlist)) {  // For std input
                    commit(m); // Sends info to server
                }
            bzero(m->buf, MAXMESSAGE + 16); // Sets buf to \0
            break;
        }
    } 
    return(0);
}

void commit (struct client *m) { // Writes stdin to server
    if (fgets(m->buf, MAXMESSAGE, stdin)) { // Gets stdin
        // Adds a network newline if the length exceeds MAXMESSAGE
        m->buf[MAXMESSAGE - 1] = (strlen(m->buf) == MAXMESSAGE - 1) ? '\n' : '\0';
        if (write(m->fd, m->buf, strlen(m->buf)) != strlen(m->buf)) { // Writes to server
            fprintf(stderr, "Error, cannot write to socket\n"); // Cannot write
            exit(1);
        }
    }
}
void receiveAndProcess(struct client *m) {
    char reply [MAXMESSAGE + MAXHANDLE];
    char *handle, *prompt, *message;
    while(!myreadline(m)) {} // Processes buffer and prints a full line when possible
    printf("%s\n",m->buf); // Prints data from server
    handle = &m->buf[0]; // Sets handle
    prompt = strchr(m->buf, ':'); // Gets Hey Marvin prompt
    message = strchr(m->buf, ','); // Gets message for processing
    if (handle && prompt && message) { // Processes input if there is a handle, colon and comma
        prompt[0] = '\0'; prompt+=2; // Changes ": Hey Marvin*" to " Hey Marvin"
        message[0] = '\0'; message++; // Changes ", 1 + 1" to " 1 + 1"
        if (strcasecmp(prompt, "hey marvin") == 0) { // Checks if hey marvin is present
            struct expr *xp = parse(message); // Parses message
            if (xp) { // If expression is valid
                sprintf(reply, "Hey %s, %d\r\n", handle, evalexpr(xp)); // Gets output
                freeexpr(xp); // Frees xp
            } else { // Error if expression isn't valid
                printf("[%s]\n", errorstatus); // Prints invalid characters
                sprintf(reply, "Hey %s, I don't like that.\r\n", handle); // Gets Error output
            }
            if (write(m->fd, reply, strlen(reply)) != strlen(reply)) { // Writes to server
                perror("write()");
                exit(1);
            }
            fflush(stdout);
        }
    }
}

char *myreadline(struct client *m)
{
    int nbytes;

    /* move the leftover data to the beginning of buf */
    if (m->bytes_in_buf && m->nextpos)
        memmove(m->buf, m->nextpos, m->bytes_in_buf);

    /* If we've already got another whole line, return it without a read() */
    if ((m->nextpos = extractline(m->buf, m->bytes_in_buf))) {
        m->bytes_in_buf -= (m->nextpos - m->buf);
        return(m->buf);
    }

    /*
     * Ok, try a read().  Note that we _never_ fill the buffer, so that there's
     * always room for a \0.
     */
    switch((nbytes = read(m->fd, m->buf + m->bytes_in_buf, sizeof m->buf - m->bytes_in_buf - 1))) {
        case -1:
            perror("read()");
            printf("Server shut down\n");
            exit(0);
        case 0: 
            perror("read()");
            printf("Server shut down\n");
            exit(0);
        default:
            m->bytes_in_buf += nbytes;
            /* So, _now_ do we have a whole line? */
            if ((m->nextpos = extractline(m->buf, m->bytes_in_buf))) {
                m->bytes_in_buf -= (m->nextpos - m->buf);
                return(m->buf);
            }
            /*
             * Don't do another read(), to avoid the possibility of blocking.
             * However, if we've hit the maximum message size, we should call
             * it all a line.
             */
            if (m->bytes_in_buf >= MAXMESSAGE) {
                m->buf[m->bytes_in_buf] = '\0';
                m->bytes_in_buf = 0;
                m->nextpos = NULL;
                return(m->buf);
            }

    }

    /* If we get to here, we don't have a full input line yet. */
    return(NULL);
}
