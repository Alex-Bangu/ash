#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <netinet/in.h>    /* struct sockaddr_in */

#include "socket.h"

void setup_server_socket(struct listen_sock *s, int port_number) {
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        perror("malloc");
        exit(1);
    }
    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(port_number);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("server socket");
	write(STDERR_FILENO, "ERROR: Builtin failed: start-server", 36);
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        perror("setsockopt");
	write(STDERR_FILENO, "ERROR: Builtin failed: start-server", 36);
        exit(1);
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
	if(errno == EADDRINUSE) {
		perror("ERROR: ");
	} else {
		perror("server: bind");
	}
        close(s->sock_fd);
	write(STDERR_FILENO, "ERROR: Builtin failed: start-server", 36);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(s->sock_fd);
	write(STDERR_FILENO, "ERROR: Builtin failed: start-server", 36);
        exit(1);
    }
}

/* Insert helper functions from last week here. */

int find_network_newline(const char *buf, int inbuf) {
    if(inbuf < 2) {
	    	// fprintf(stderr, "inbuf too small\n");
		return -1;
	}

	int i = 0;
	while(i < inbuf - 1) {
		if(buf[i] == '\r' && buf[i + 1] == '\n') {
			return i + 2;
		} else {
			i++;
		}
	}
	// fprintf(stderr, "fnn failed\n");
    	return -1;
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
	int nbytes;
	int clrf = 0;
	// printf("inbuf: %d, BUF_SIZE - 1: %d\n", *inbuf, BUF_SIZE - 1);
	if(*inbuf >= BUF_SIZE - 1) {
		// printf("Message size exceeded\n");
		return -1; // message size exceeded
	}

	while((nbytes = read(sock_fd, buf + *inbuf, 1)) > 0) {
		*inbuf += nbytes;
		clrf = find_network_newline(buf, *inbuf);
		if(clrf > 0) {
			return 0; // clrf terminator found
		}
	}
	if(nbytes == -1) {
		return -1; // read error
	} else if(nbytes == 0) {
		return 1; // socket closed
	}
	if(*inbuf > 0) {
		return 2;
	} else { 
		return 0;
	}
}

int get_message(char **dst, char *src, int *inbuf) {
    	int check = find_network_newline(src, *inbuf);
	if(check == -1) {
		// fprintf(stderr, "ERROR: get_message(): No network newline in src\n");
		return 1;
	}

	char *buf = malloc(sizeof(char) * (check - 1));
	if(buf == NULL) {
		return 1;
	}
	memcpy(buf, src, check - 2);
	buf[check - 2] = '\0';
	// printf("buf: %s\n", buf);
        *dst = buf;

	int leftover = *inbuf - check;
	if(leftover > 0) {
		memmove(src, src + check, leftover);
	}
	memset(src + leftover, '\0', *inbuf - leftover);

	*inbuf = leftover;

    	return 0;
}

/* Helper function to be completed for this week. */
/*
 * Write a string to a socket.
 *
 * Return 0 on success.
 * Return 1 on error.
 * Return 2 on disconnect.
 * 
 * See Robert Love Linux System Programming 2e p. 37 for relevant details
 */
int write_to_socket(int sock_fd, char *buf, int len) {
    int total_written = 0;
    int nbytes;
    while(total_written < len) {
	    nbytes = write(sock_fd, buf + total_written, len - total_written);
	    if(nbytes < 0) {
		    if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
			    continue;
		    } else {
			    // printf("write error\n");
			    return 1;
		    }
	    } else if(nbytes == 0) {
		    // printf("disconnect\n");
		    return 2;
	    }

	    total_written += nbytes;
    }
    return 0;

}
