#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

char *test_cases[] = {
    "login.txt",
    "wrong-login-password.txt",
    "add-admin.txt",
    "add-user.txt",
    "date-ls.txt",
    "cat-elapsed.txt",
    "update-username.txt",
    "update-password.txt",
    "no-permissions.txt",
    "update-role.txt",
    "no-permissions-2.txt",
    "delete-user.txt",
    "user-deleted.txt",
};
int test_cases_length = sizeof(test_cases)/sizeof(test_cases[0]);

bool should_run_test(int test_index, int argc, char **argv) {
    if (argc == 1) return true;
    for (int i = 1; i < argc; i++) {
        if (atoi(argv[i]) == test_index)
            return true;
    }
    return false;
}

int main(int argc, char **argv) {
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

    sleep(2); // wait for server to startup
    // run the clients
    for (int i = 0; i < test_cases_length; i++) {
        if (!should_run_test(i, argc, argv)) continue;
        char input_file_name [256] = {0};
        char output_file_name[256] = {0};
        snprintf(input_file_name,  256, "./tests/inputs/%02d-%s",  i, test_cases[i]);
        snprintf(output_file_name, 256, "./tests/outputs/%02d-%s", i, test_cases[i]);
        int input_file_fd  = open(input_file_name, O_RDONLY);
        int output_file_fd = open(output_file_name, O_WRONLY | O_CREAT, 0644);

        printf("testcase [%d]: %s\n", i, test_cases[i]);
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
        waitpid(client, NULL, 0);
    }

    kill(server, SIGKILL);

    return 0;
}