#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "socket.h"

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    (void)argc;
    struct server_sock s;
    s.inbuf = 0;
    int exit_status = 0;
    
    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(strtol(argv[1], NULL, 10));
    if (inet_pton(AF_INET, argv[2], &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(s.sock_fd);
        exit(1);
    }

    char *buf = NULL; // Buffer to read name from stdin
    int name_valid = 1;
    while(!name_valid) {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        int name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0) {
            perror("getline");
            fprintf(stderr, "Error reading username.\n");
            free(buf);
            exit(1);
        }
        
        if (name_len - 1 > MAX_NAME) { // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
        }
        else {
            // Replace LF+NULL with CR+LF
            buf[name_len-1] = '\r';
            buf[name_len] = '\n';
            if (write_to_socket(s.sock_fd, buf, name_len+1)) {
                fprintf(stderr, "Error sending username.\n");
                free(buf);
                exit(1);
            }
            name_valid = 1;
            free(buf);
        }
    }
    
    /*
     * See here for why getline() is used above instead of fgets():
     * https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152445
     */
    
    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(s.sock_fd, &fds);
    buf = malloc(sizeof(char) * (MAX_USER_MSG + 3));
    buf[0] = '\0';
    s.buf[0] = '\0';
    char *to_display;
    int s_val;
    int nbytes;
    FILE *sock_f = fdopen(s.sock_fd, "r");

    
    /*
     * Step 2: Using select, monitor the socket for incoming mesages
     * from the server and stdin for data typed in by the user.
     */
    while(1) {
	s_val = select(s.sock_fd + 1, &fds, NULL, NULL, NULL);
	if(s_val == -1) {
		perror("client: select");
		exit(1);
        
        /*
         * Step 3: Read user-entered message from the standard input
         * stream. We should read at most MAX_USR_MSG bytes at a time.
         * If the user types in a message longer than MAX_USR_MSG,
         * we should leave the rest of the message in the standard
         * input stream so that we can read it later when we loop
         * back around.
         * 
         * In other words, an oversized messages will be split up
         * into smaller messages. For example, assuming that
         * MAX_USR_MSG is 10 bytes, a message of 22 bytes would be
         * split up into 3 messages of length 10, 10, and 2,
         * respectively.
         * 
         * It will probably be easier to do this using a combination of
         * fgets() and ungetc(). You probably don't want to use
         * getline() as was used for reading the user name, because
         * that would read all the bytes off of the standard input
         * stream, even if it exceeds MAX_USR_MSG.
         */
	} else if(FD_ISSET(STDIN_FILENO, &fds)) {
		fgets(buf, MAX_USER_MSG, stdin);
		strcat(buf, "\r\n");
		fd_set fds_stdin;
		struct timeval timeout = {0, 0};
		FD_ZERO(&fds_stdin);
		FD_SET(STDIN_FILENO, &fds_stdin);
		if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout) > 0) { // if code enters this if, then there's more stuff in stdin
			free(buf);
    			fclose(sock_f);
    			close(s.sock_fd);
    			exit(exit_status); // killing client
		}
		write_to_socket(s.sock_fd, buf, strlen(buf));
		memset(buf, '\0', MAX_USER_MSG + 3);
		

        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
	} else if(FD_ISSET(s.sock_fd, &fds)) {
		if((nbytes = read_from_socket(s.sock_fd, s.buf, &s.inbuf)) == 0) {
			get_message(&to_display, s.buf, &s.inbuf);	
			write(STDOUT_FILENO, to_display, strnlen(to_display, 128));
			fflush(stdout);
			free(to_display);
		} else if(nbytes == 1) {
			continue;
		}
	}

	FD_ZERO(&fds);
   	FD_SET(STDIN_FILENO, &fds);
    	FD_SET(s.sock_fd, &fds);
    }
    free(buf);
    fclose(sock_f);
    close(s.sock_fd);
    exit(exit_status);
}
