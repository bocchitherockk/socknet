#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#define CLIENTS_COUNT 5

int main(void) {
    // run the server
    int server = fork();
    if (server == -1) {
        perror("fork");
        exit(1);
    } else if (server == 0) {
        execlp("./server/server_main", "server_main", NULL);
        perror("execl");
        exit(1);
    }

    // run the clients
    for (int i = 0; i < CLIENTS_COUNT; i++) {
        char input_file_name [256] = {0};
        char output_file_name[256] = {0};
        snprintf(input_file_name,  256, "./tests/inputs/input-%d.txt", i);
        snprintf(output_file_name, 256, "./tests/outputs/output-%d.txt", i);
        int input_file_fd  = open(input_file_name, O_RDONLY);
        int output_file_fd = open(output_file_name, O_WRONLY | O_CREAT, 0644);

        int client = fork();
        if (client == -1) {
            perror("fork");
            exit(1);
        } else if (client == 0) { // child main.exe
            dup2(input_file_fd, STDIN_FILENO);
            dup2(output_file_fd, STDOUT_FILENO);
            execlp("./client/client_main", "client_main", NULL);
            perror("execlp");
            exit(1);
        }
    }

    // wait for all clients to finish running
    for (int i = 0; i < CLIENTS_COUNT; i++) {
        wait(NULL);
    }

    return 0;
}