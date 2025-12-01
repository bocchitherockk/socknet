#include <stdio.h> // perror
#include <stdlib.h> // exit
#include <unistd.h> // fork, execl
#include <signal.h> // kill, SIGKILL
#include <sys/wait.h> // waitpid

int main(void) {
    int server = fork();
    if (server == -1) {
        perror("fork");
        exit(1);
    } else if (server == 0) {
        if (execl("./server/server_main", "server_main", NULL) == -1) {
            perror("execl");
            exit(1);
        }
    }

    int client = fork();
    if (client == -1) {
        perror("fork");
        exit(1);
    } else if (client == 0) {
        if (execl("./client/client_main", "client_main", NULL) == -1) {
            perror("execl");
            exit(1);
        }
    }

    if (waitpid(client, NULL, 0) == -1) {
        perror("waitpid");
        exit(1);        
    }

    if (kill(server, SIGKILL) == -1) {
        perror("kill");
        exit(1);
    }
    return 0;
}