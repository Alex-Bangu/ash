#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/prctl.h>

#include "socket.h"
#include "chat_helpers.h"
// #include "io_helpers.h"

int sigint_received = 0;
int total_clients = 0;
int curr_clients = 0;


void sigint_handler(int code) {
    (void)code;
    sigint_received = 1;
}

/*
 * Wait for and accept a new connection.
 * Return -1 if the accept call failed.
 */
int accept_connection(int fd, struct client_sock **clients) {
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int num_clients = 0;
    struct client_sock *curr = *clients;
    while (curr != NULL && curr->next != NULL) {
        curr = curr->next;
        num_clients++;
    }

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (num_clients == MAX_CONNECTIONS) {
        close(client_fd);
        return -1;
    }

    struct client_sock *newclient = malloc(sizeof(struct client_sock));
    newclient->sock_fd = client_fd;
    newclient->inbuf = newclient->state = 0;
    newclient->username = NULL;
    newclient->next = NULL;
    memset(newclient->buf, 0, BUF_SIZE);
    if (*clients == NULL) {
        *clients = newclient;
    }
    else {
        curr->next = newclient;
    }

    return client_fd;
}

/*
 * Close all sockets, free memory, and exit with specified exit status.
 */
void clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status) {
    struct client_sock *tmp;
    while (clients) {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp->username);
        free(tmp);
    }
    close(s.sock_fd);
    free(s.addr);
    exit(exit_status);
}

int main(int argc, char *argv[]) {
	(void)argc;
    if (prctl(PR_SET_PDEATHSIG, SIGINT) == -1) {
        perror("prctl");
        exit(1);
    }
    // This line causes stdout not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    
    /*
     * Turn off SIGPIPE: write() to a socket that is closed on the other
     * end will return -1 with errno set to EPIPE, instead of generating
     * a SIGPIPE signal that terminates the process.
     */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    // Linked list of clients
    struct client_sock *clients = NULL;
    
    struct listen_sock s;
    setup_server_socket(&s, strtol(argv[1], NULL, 10));
    
    // Set up SIGINT handler
    struct sigaction sa_sigint;
    memset (&sa_sigint, 0, sizeof (sa_sigint));
    sa_sigint.sa_handler = sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);
    
    int exit_status = 0;
    
    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;
    
    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    do {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (sigint_received) break;
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("server: select");
            exit_status = 1;
            break;
        }

        /* 
         * If a new client is connecting, create new
         * client_sock struct and add to clients linked list.
         */
        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            int client_fd = accept_connection(s.sock_fd, &clients);
            if (client_fd < 0) {
                // printf("Failed to accept incoming connection.\n");     REMOVED PRINT
                continue;
            }
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
	    curr_clients++;
            // printf("Accepted connection\n");    
        }

        if (sigint_received) break;

        /*
         * Accept incoming messages from clients,
         * and send to all other connected clients.
         */
        struct client_sock *curr = clients;
        while (curr) {
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {
                curr = curr->next;
                continue;
            }
            int client_closed = read_from_client(curr);
	    struct timeval timeout = {0, 0};
	    int select_check = 0;
	    fd_set fd_excess;
	    FD_ZERO(&fd_excess);
    	    FD_SET(curr->sock_fd, &fd_excess);
	    if ((select_check = select(curr->sock_fd + 1, &fd_excess, NULL, NULL, &timeout)) > 0) { // if code enters this if, then there's more stuff in sock_fd
		char temp[2];
		memset(temp, '\0', 2);
		if((read(curr->sock_fd, temp, 1)) > 0) { // checks to see if there is actually excess or if the socket is just closed
			client_closed = 1;
		}
	    }

            
            // If error encountered when receiving data
            if (client_closed == -1) {
		// printf("disconnected client\n");
                client_closed = 1; // Disconnect the client
            }
            
            // If received at least one complete message
            // and client is newly connected: Get username
            if (client_closed == 0 && curr->username == NULL) {
                if (set_username(curr)) {
                    // printf("Error processing user name from client %d.\n", curr->sock_fd);     
                    client_closed = 1; // Disconnect the client
                }
                else {
                    // printf("Client %d user name is %s.\n", curr->sock_fd, curr->username);    
                }
            }
                
            char *msg;
            // Loop through buffer to get complete message(s)
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf))) {
                // printf("Echoing message from %s.\n", curr->username);                      
                char write_buf[BUF_SIZE];
		int nl_check = 0;
		write_buf[0] = '\0';
		if (strncmp(msg, "#SHELL:", 7) == 0) {
       			 strncat(write_buf, msg + 7, MAX_USER_MSG);
			 strncat(write_buf, "\n", 2);
			 curr_clients--;
		} else if((strncmp(msg, "\\connected", 10)) == 0) {
			sprintf(write_buf, "%d\n", curr_clients);
		} else {
                	strncat(write_buf, curr->username, MAX_NAME);
			strncat(write_buf, " ", 2);
                	strncat(write_buf, msg, MAX_USER_MSG);
		}
		if(write_buf[strnlen(write_buf, 128) - 1] != '\n') {
			printf("DEBUG: no newline found\n");
			nl_check = 1;
		}	
		write(STDOUT_FILENO, write_buf, strnlen(write_buf, 128));
		if(nl_check) {
			write(STDOUT_FILENO, "\n", 2);
		}
                free(msg);
                int data_len = strlen(write_buf);
                
                struct client_sock *dest_c = clients;
                while (dest_c) {
			int ret = write_buf_to_client(dest_c, write_buf, data_len);
			if(nl_check) {
				char nl_buf[4];
				nl_buf[0] = '\n';
				write_buf_to_client(dest_c, nl_buf, 1);
			}
                        if (ret == 0) {
                            // printf("Sent message from %s (%d) to %s (%d).\n",
                                // curr->username, curr->sock_fd,                               REMOVED PRINT
                                // dest_c->username, dest_c->sock_fd);
                        } else {
                            // printf("Failed to send message to user %s (%d).\n", dest_c->username, dest_c->sock_fd);    REMOVED PRINT
                            if (ret == 2) {
                                // printf("User %s (%d) disconnected.\n", dest_c->username, dest_c->sock_fd);      REMOVED PRINT
                                close(dest_c->sock_fd);
                                FD_CLR(dest_c->sock_fd, &all_fds);
                                assert(remove_client(&dest_c, &clients) == 0); // If this fails we have a bug
                                continue;
                            }
                        }
                    dest_c = dest_c->next;
		}
	    } 
            
            if (client_closed == 1) { // Client disconnected
                // Note: Never reduces max_fd when client disconnects
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                // printf("Client %d disconnected\n", curr->sock_fd);                                           REMOVED PRINT
                assert(remove_client(&curr, &clients) == 0); // If this fails we have a bug
            }
            else {
                curr = curr->next;
            }
        }
    } while(!sigint_received);
    
    clean_exit(s, clients, exit_status);
}
