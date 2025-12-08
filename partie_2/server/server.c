#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../shared.h"
#include "./server.h"
#include "./Users.h"

static int server_socket_fd = 0;

static Users users = {0};

typedef enum Menu_Option {
    MENU_OPTION_DATE = 1,
    MENU_OPTION_LS,
    MENU_OPTION_CAT,
    MENU_OPTION_ELAPSED,
    MENU_OPTION_EXIT,
} Menu_Option;

static char *_Server_run_command(const char *executable, char *args[]) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(1);
    }

    int process = fork();
    if (process == -1) {
        perror("fork");
        exit(1);
    } else if (process == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        execvp(executable, args);
        perror("execvp failed");
        exit(1);
    }

    size_t result_size = 1024 * 3; // 3 mb
    char *result = calloc(result_size, sizeof(char));
    if (read(pipe_fd[0], result, result_size) == -1) {
        perror("read");
        exit(1);
    }

    return result;
}

void Server_send_menu(int client_socket_fd) {
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, (
        "=================\n"
        "choose an option:\n"
        "    1. date\n"
        "    2. ls <dirname>\n"
        "    3. cat <filename>\n"
        "    4. elapsed\n"
        "    5. exit\n"
        "what is your choice: "
    ));
}

Menu_Option Server_read_menu_option(int client_socket_fd) {
    Menu_Option option = 0;
    if (recv(client_socket_fd, &option, sizeof(option), 0) == -1) {
        perror("recv");
        exit(1);
    }
    return option;
}

void Server_handle_option_date(int client_socket_fd) {
    char *result = _Server_run_command("date", (char *[]){"date", "+%Y/%m/%d %H:%M:%S", NULL});
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    free(result);
}

void Server_handle_option_ls(int client_socket_fd) {
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "Enter the directory name: ");

    Shared_send_command(client_socket_fd, COMMAND_INPUT_STRING);
    char *filename = Shared_read_string(client_socket_fd);
    char *result = _Server_run_command("ls", (char *[]){"ls", "-l", filename, NULL});

    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    free(result);
}

void Server_handle_option_cat(int client_socket_fd) {
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "Enter the filename: ");

    Shared_send_command(client_socket_fd, COMMAND_INPUT_STRING);
    char *filename = Shared_read_string(client_socket_fd);
    char *result = _Server_run_command("cat", (char *[]){"cat", filename, NULL});

    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    free(result);
}

void Server_handle_option_elapsed(int client_socket_fd, time_t login_time) {
    time_t elapsed_time = time(NULL) - login_time;
    int elapsed_seconds = elapsed_time % 60;
    int elapsed_minutes = (elapsed_time / 60) % 60;
    int elapsed_hours   = (elapsed_time / 60 / 60) % 24;
    int elapsed_days    = (elapsed_time / 60 / 60 / 24) % 30;
    int elapsed_months  = (elapsed_time / 60 / 60 / 24 / 30) % 12;
    int elapsed_years   = (elapsed_time / 60 / 60 / 24 / 30 / 12);
    if (elapsed_minutes < 1) elapsed_minutes = 0;
    if (elapsed_hours   < 1) elapsed_hours   = 0;
    if (elapsed_days    < 1) elapsed_days    = 0; 
    if (elapsed_months  < 1) elapsed_months  = 0;
    if (elapsed_years   < 1) elapsed_years   = 0;

    char result[100] = {0};
    snprintf(result, 100, "%s%s%s%02d:%02d:%02d\n",
        elapsed_years == 0                                             ? "" : "years: %d, ",
        elapsed_years == 0 && elapsed_months == 0                      ? "" : "months: %d, ",
        elapsed_years == 0 && elapsed_months == 0 && elapsed_days == 0 ? "" : "days: %d ",
        elapsed_hours,
        elapsed_minutes,
        elapsed_seconds
    );
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
}

void Server_handle_option_exit(int client_socket_fd) {
    Shared_send_command(client_socket_fd, COMMAND_DISCONNECT);
}

void Server_handle_option_invalid(int client_socket_fd) {
    Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "invalid Menu_Option\n");
}


void Server_handle_client_sync(int client_socket_fd) {
    User *user = NULL;
    while (true) {
        Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Enter your username: ");
        Shared_send_command(client_socket_fd, COMMAND_INPUT_STRING);
        
        char *username = Shared_read_string(client_socket_fd);
        user = Users_find_user_by_username(&users, username);
        if (user != NULL) break;
        Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "No account found with this username\n");
    }
    while (true) {
        Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Enter your password: ");
        Shared_send_command(client_socket_fd, COMMAND_INPUT_STRING);

        char *password = Shared_read_string(client_socket_fd);
        bool login = User_login(user, password);
        if (login == true) break;
        Shared_send_command(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Wrong password! try again\n");
    }

    Menu_Option option;
    do {
        Server_send_menu(client_socket_fd);
        Shared_send_command(client_socket_fd, COMMAND_INPUT_INT);
        option = Server_read_menu_option(client_socket_fd);
    
        switch (option) {
            case MENU_OPTION_DATE   : Server_handle_option_date(client_socket_fd);                      break;
            case MENU_OPTION_LS     : Server_handle_option_ls(client_socket_fd);                        break;
            case MENU_OPTION_CAT    : Server_handle_option_cat(client_socket_fd);                       break;
            case MENU_OPTION_ELAPSED: Server_handle_option_elapsed(client_socket_fd, user->login_time); break;
            case MENU_OPTION_EXIT   : Server_handle_option_exit(client_socket_fd);                      break;
            default                 : Server_handle_option_invalid(client_socket_fd);
        }

    } while (option != MENU_OPTION_EXIT);
}

void Server_init() {
    printf("Server running ...\n");
    srand(time(NULL) ^ getpid());

    Users_append(&users, User_create("admin", "admin", USER_ROLE_ADMIN));

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_address = {
        .sin_addr = {0},
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Error: unable to parse SERVER_IP_ADDRESS '%s'\n", SERVER_IP_ADDRESS);
        exit(1);
    }

    // bind socket to port
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind");
        exit(1);
    }

    // listen for connections
    if (listen(server_socket_fd, 10) == -1) {
        perror("listen");
        exit(1);
    }
}

void Server_destroy() {
    if (close(server_socket_fd) == -1) {
        perror("close");
        exit(1);
    }
    printf("Server stopped\n");
}

int Server_accept_conection(void) {
    int client_socket_fd = accept(server_socket_fd, NULL, NULL);
    if (client_socket_fd == -1) {
        perror("accept");
        exit(1);
    }
    return client_socket_fd;
}

void Server_handle_client_async(int client_socket_fd) {
    int worker_pid = fork();
    if (worker_pid == -1) {
        perror("fork");
        exit(1);
    } else if (worker_pid == 0) {
        Server_handle_client_sync(client_socket_fd);
    }
}
