#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "socket.h"


void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        // fprintf(stderr, "Usage: %s <port> <hostname> <message>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    char *hostname = argv[2];
    char *message = malloc(sizeof(char) * (strlen(argv[3]) + 3));
    memset(message, '\0', strlen(argv[3] + 3));
    sprintf(message, "%s\r\n", argv[3]);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        error_exit("socket");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        error_exit("inet_pton");
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error_exit("connect");
    }

    // Modify message to include special prefix so the server knows it's from send.c
    char modified_msg[BUF_SIZE];
    memset(modified_msg, '\0', BUF_SIZE);
    snprintf(modified_msg, BUF_SIZE, "#SHELL:%s", message);
    free(message);
    write_to_socket(sock_fd, modified_msg, strlen(modified_msg));
    memset(modified_msg, '\0', BUF_SIZE);
    close(sock_fd);
    exit(0);
}

