#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./shared.h"

void Shared_send_int(int socket_fd, int x) {
    if (send(socket_fd, &x, sizeof(x), 0) == -1) {
        perror("send");
        exit(1);
    }
}

int Shared_read_int(int socket_fd) {
    int x = 0;
    if (recv(socket_fd, &x, sizeof(x), 0) == -1) {
        perror("recv");
        exit(1);
    }
    return x;
}

void Shared_send_string(int socket_fd, char *string) {
    size_t string_length = strlen(string);
    if (send(socket_fd, &string_length, sizeof(string_length), 0) == -1) {
        perror("send");
        exit(1);
    }
    if (send(socket_fd, string, string_length, 0) == -1) {
        perror("send");
        exit(1);
    }
}

char *Shared_read_string(int socket_fd) {
    size_t buffer_length = 0;
    if (recv(socket_fd, &buffer_length, sizeof(buffer_length), 0) == -1) {
        perror("recv");
        exit(1);
    }

    char *buffer = calloc(buffer_length + 1, sizeof(char));
    if (buffer == NULL) {
        perror("calloc");
        exit(1);
    }

    if (recv(socket_fd, buffer, buffer_length, 0) == -1) {
        perror("recv");
        exit(1);
    }
    buffer[buffer_length] = '\0';
    return buffer;
}