#include <stdio.h> // perror
#include <stdlib.h> // exit, malloc
#include <string.h>
#include <stdbool.h>
#include <unistd.h> // write, read, getpid, pause, close, access
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../shared.h"
#include "./client.h"


static int client_socket_fd = 0;


void Client_init() {
    srand(time(NULL) ^ getpid());
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
}

void Client_destroy() {
    if (close(client_socket_fd) == -1) {
        perror("close");
        exit(1);
    }
}

void Client_connect() {
    struct sockaddr_in server_address = {
        .sin_addr = {0},
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Error: unable to parse SERVER_IP_ADDRESS '%s'\n", SERVER_IP_ADDRESS);
        exit(1);
    }

    // connect to the server
    if (connect(client_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("connect");
        exit(1);
    }
}

void Client_run() {
    Command command = 0;
    while (command != COMMAND_DISCONNECT) {
        command = Shared_read_int(client_socket_fd);
        switch (command) {
            case COMMAND_DISPLAY: {
                char *string = Shared_read_string(client_socket_fd);
                printf("%s", string); fflush(stdout);
                free(string);
            } break;
            case COMMAND_INPUT_STRING: {
                char input[256] = {0};
                scanf("%s", input);
                Shared_send_string(client_socket_fd, input);
            } break;
            case COMMAND_INPUT_SIZE_T: {
                size_t a = 0;
                scanf("%zu", &a);
                if (send(client_socket_fd, &a, sizeof(a), 0) == -1) {
                    perror("send");
                    exit(1);
                }
            } break;
            case COMMAND_INPUT_INT: {
                int a = 0;
                scanf("%d", &a);
                if (send(client_socket_fd, &a, sizeof(a), 0) == -1) {
                    perror("send");
                    exit(1);
                }
            } break;
            case COMMAND_DISCONNECT: {
                continue;
            }
            default: {
                fprintf(stderr, "ERROR: invalid command from server: %d\n", command);
                exit(1);
            }
        }
    }
}