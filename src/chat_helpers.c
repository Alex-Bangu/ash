#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "socket.h"
#include "chat_helpers.h"

int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    char *null;
    int i = 0;
    while(i <= len) {
	    if(buf[i] == '\0') {
		    null = &buf[i];
		    break;
	    }
	    else {
		    i++;
	    }
    }
    if(null == NULL) {
	    fprintf(stderr, "memchr error\n");
	    return 1;
    } else if(len + 2 > BUF_SIZE) {
	    fprintf(stderr, "bufsize error\n");
	    return 1;
    }
    buf[len] = '\r';
    buf[len + 1] = '\n';
    buf[len + 2] = '\0';
    len += 2;
    // fprintf(stderr, "buf_to_client: %s\n", buf);
    return write_to_socket(c->sock_fd, buf, len);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
	if(!clients || !*clients || !curr || !*curr) {
		return 1;
	}

    struct client_sock *prev = NULL;
    struct client_sock *temp = *clients;
    
    while (temp != NULL && temp != *curr) {
        prev = temp;
        temp = temp->next;
    }
    
    if (temp == NULL) {
        return 1;
    }
    
    if (prev != NULL) {
        prev->next = temp->next;
    } else {
        *clients = temp->next;
    }
    if(*curr == temp) {
	    *curr = temp->next;
    }
    close(temp->sock_fd);
    
    if(temp->username) {
	    free(temp->username);
    }

    free(temp);
    curr_clients--;
    
    return 0;
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

int set_username(struct client_sock *curr) {
	char *username = malloc(sizeof(char) * 15);
	total_clients++;
	sprintf(username, "client%d:", total_clients);
	curr->username = username;

    return 0;
}



