#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../shared.h"
#include "./server.h"
#include "./Users.h"

static int server_socket_fd = 0;

pthread_mutex_t users_mutex = {0};
static Users users = {0};

typedef enum Menu_Option {
    MENU_OPTION_ADD_USER = 1,
    MENU_OPTION_DELETE_USER,
    MENU_OPTION_UPDATE_ROLE,

    MENU_OPTION_UPDATE_USERNAME,
    MENU_OPTION_UPDATE_PASSWORD,

    MENU_OPTION_DATE,
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
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execvp(executable, args);
        perror("execvp failed");
        exit(1);
    }

    waitpid(process, NULL, 0);
    size_t result_size = 1024 * 3; // 3 kb
    char *result = calloc(result_size, sizeof(char));
    if (read(pipe_fd[0], result, result_size) == -1) {
        perror("read");
        exit(1);
    }
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    return result;
}

void Server_send_menu(int client_socket_fd) {
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, (
        "\n\033[1;36m=================================================\n"
        "              MAIN MENU                         \n"
        "=================================================\033[0m\n\n"
        "\033[1;33m[USER MANAGEMENT]\033[0m\n"
        "  1. Add User        - Create new account (Admin)\n"
        "  2. Delete User     - Remove account (Admin)\n"
        "  3. Update Role     - Change permissions (Admin)\n\n"
        "\033[1;33m[ACCOUNT SETTINGS]\033[0m\n"
        "  4. Update Username - Change your username\n"
        "  5. Update Password - Change your password\n\n"
        "\033[1;33m[SYSTEM COMMANDS]\033[0m\n"
        "  6. Date            - Show current date/time\n"
        "  7. List Files      - Show directory contents\n"
        "  8. View File       - Display file contents\n"
        "  9. Session Time    - Show time logged in\n\n"
        "  10. Exit           - Log out\n\n"
        "\033[1;32m>>> Enter your choice (1-10): \033[0m"
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

void Server_handle_option_add_user(int client_socket_fd, User_Role admin_role) {
    if (admin_role != USER_ROLE_ADMIN) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\n\033[1;31m[ERROR]\033[0m Access denied: Only administrators can add users.\n\n");
        return;
    }

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Create New User ===\033[0m\n");
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "Username: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *username = Shared_read_string(client_socket_fd);

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "Password: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *password = Shared_read_string(client_socket_fd);

    int tries = 0;
    int max_tries = 3;
    char *confirmation_password = NULL;
    while (tries < max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Confirm Password: ");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
        confirmation_password = Shared_read_string(client_socket_fd);
        if (strcmp(confirmation_password, password) == 0) break;
        tries++;
        free(confirmation_password);
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;33m[WARNING]\033[0m Passwords don't match. Try again.\n");

    }
    if (tries >= max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Too many failed attempts. Operation cancelled.\n\n");
        return;
    }

    User_Role role = 0;
    while (role != USER_ROLE_ADMIN && role != USER_ROLE_USER) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\nSelect Role:\n  [1] Admin  (full access)\n  [2] User   (limited access)\n\033[1;32mChoice: \033[0m");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_INT);
        role = Shared_read_int(client_socket_fd);
        if (role != USER_ROLE_ADMIN && role != USER_ROLE_USER) {
            Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
            Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Invalid role. Please enter 1 or 2.\n");
        }
    }

    User new_user = User_create(username, password, role);

    pthread_mutex_lock(&users_mutex);
    Users_append(&users, new_user);
    pthread_mutex_unlock(&users_mutex);

    free(username);
    free(password);
    free(confirmation_password);
}

void Server_handle_option_delete_user(int client_socket_fd, User_Role admin_role) {
    if (admin_role != USER_ROLE_ADMIN) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\n\033[1;31m[ERROR]\033[0m Access denied: Only administrators can delete users.\n\n");
        return;
    }

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Delete User ===\033[0m\nUsername: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *username = Shared_read_string(client_socket_fd);
    pthread_mutex_lock(&users_mutex);
    User *user_to_delete = Users_find_user_by_username(&users, username);
    // TODO: add check if user_to_delete == NULL
    size_t delete_index = user_to_delete - users.items;
    Users_remove_at(&users, delete_index);
    pthread_mutex_unlock(&users_mutex);
    free(username);
}

void Server_handle_option_update_role(int client_socket_fd, User_Role admin_role) {
    if (admin_role != USER_ROLE_ADMIN) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\n\033[1;31m[ERROR]\033[0m Access denied: Only administrators can change roles.\n\n");
        return;
    }

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Update User Role ===\033[0m\nUsername: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *username = Shared_read_string(client_socket_fd);

    User_Role role = 0;
    while (role != USER_ROLE_ADMIN && role != USER_ROLE_USER) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\nSelect New Role:\n  [1] Admin  (full access)\n  [2] User   (limited access)\n\033[1;32mChoice: \033[0m");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_INT);
        role = Shared_read_int(client_socket_fd);
        if (role != USER_ROLE_ADMIN && role != USER_ROLE_USER) {
            Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
            Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Invalid role. Please enter 1 or 2.\n");
        }
    }    

    pthread_mutex_lock(&users_mutex);
    User *user = Users_find_user_by_username(&users, username);
    user->role = role;
    pthread_mutex_unlock(&users_mutex);
    free(username);
}

void Server_handle_option_update_username(int client_socket_fd, User *user) {
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Update Username ===\033[0m\nNew Username: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *new_username = Shared_read_string(client_socket_fd);
    strncpy(user->username, new_username, sizeof(user->username));
    free(new_username);
}

void Server_handle_option_update_password(int client_socket_fd, User *user) {
    int tries = 0;
    int max_tries = 3;
    
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Change Password ===\033[0m\n");
    
    char *old_password = NULL;
    while (tries < max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Current Password: ");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
        old_password = Shared_read_string(client_socket_fd);
        if (strcmp(old_password, user->password) == 0) break;
        tries++;
        free(old_password);
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;33m[WARNING]\033[0m Incorrect password. Try again.\n");
    }

    if (tries >= max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Too many failed attempts. Operation cancelled.\n\n");
        return;
    }

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "New Password: ");
    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *new_password = Shared_read_string(client_socket_fd);

    
    tries = 0;
    max_tries = 3;
    char *confirmation_password = NULL;
    while (tries < max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Confirm New Password: ");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
        confirmation_password = Shared_read_string(client_socket_fd);
        if (strcmp(confirmation_password, new_password) == 0) break;
        tries++;
        free(confirmation_password);
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;33m[WARNING]\033[0m Passwords don't match. Try again.\n");

    }
    if (tries >= max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Too many failed attempts. Operation cancelled.\n\n");
        return;
    }

    strncpy(user->password, new_password, sizeof(user->password));
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\033[1;32m[SUCCESS]\033[0m Password updated successfully!\n\n");

    free(old_password);
    free(new_password);
    free(confirmation_password);
}

void Server_handle_option_date(int client_socket_fd) {
    char *result = _Server_run_command("date", (char *[]){"date", "+%Y/%m/%d %H:%M:%S", NULL});
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Current Date/Time ===\033[0m\n");
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
    free(result);
}

void Server_handle_option_ls(int client_socket_fd) {
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== List Directory Contents ===\033[0m\nPath: ");

    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *filename = Shared_read_string(client_socket_fd);
    char *result = _Server_run_command("ls", (char *[]){"ls", "-l", filename, NULL});

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
    free(result);
}

void Server_handle_option_cat(int client_socket_fd) {
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== View File Contents ===\033[0m\nFilename: ");

    Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
    char *filename = Shared_read_string(client_socket_fd);
    char *result = _Server_run_command("cat", (char *[]){"cat", filename, NULL});

    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
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
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=== Session Duration ===\033[0m\n");
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, result);
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n");
}

void Server_handle_option_exit(int client_socket_fd) {
    Shared_send_int(client_socket_fd, COMMAND_DISCONNECT);
}

void Server_handle_option_invalid(int client_socket_fd) {
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;31m[ERROR]\033[0m Invalid option. Please select 1-10.\n\n");
}


void Server_handle_client_sync(int client_socket_fd) {
    User *user = NULL;
    
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\n\033[1;36m=================================================\n"
                                           "     WELCOME TO SOCKNET SERVER                 \n"
                                           "=================================================\033[0m\n\n");
    
    while (true) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;35m=== Login ===\033[0m\nUsername: ");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);
        
        char *username = Shared_read_string(client_socket_fd);
        // TODO: a better approach is to use ids instead of pointers
        pthread_mutex_lock(&users_mutex);
        user = Users_find_user_by_username(&users, username);
        pthread_mutex_unlock(&users_mutex);
        free(username);
        if (user != NULL) break;
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m User not found. Please try again.\n\n");
    }

    int tries = 0;
    int max_tries = 3;
    while (tries < max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "Password: ");
        Shared_send_int(client_socket_fd, COMMAND_INPUT_STRING);

        char *password = Shared_read_string(client_socket_fd);
        bool login = User_login(user, password);
        free(password);
        if (login == true) break;
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;33m[WARNING]\033[0m Incorrect password. Try again.\n\n");
        tries++;
    }

    if (tries >= max_tries) {
        Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
        Shared_send_string(client_socket_fd, "\033[1;31m[ERROR]\033[0m Too many failed login attempts. Connection closed.\n\n");
        Server_handle_option_exit(client_socket_fd);
        return;
    }
    
    Shared_send_int(client_socket_fd, COMMAND_DISPLAY);
    Shared_send_string(client_socket_fd, "\033[1;32m[SUCCESS]\033[0m Login successful! Welcome.\n\n");

    Menu_Option option;
    do {
        Server_send_menu(client_socket_fd);
        Shared_send_int(client_socket_fd, COMMAND_INPUT_INT);
        option = Server_read_menu_option(client_socket_fd);
    
        switch (option) {
            case MENU_OPTION_ADD_USER   : Server_handle_option_add_user(client_socket_fd, user->role); break;
            case MENU_OPTION_DELETE_USER: Server_handle_option_delete_user(client_socket_fd, user->role); break;
            case MENU_OPTION_UPDATE_ROLE: Server_handle_option_update_role(client_socket_fd, user->role); break;

            case MENU_OPTION_UPDATE_USERNAME: Server_handle_option_update_username(client_socket_fd, user); break;
            case MENU_OPTION_UPDATE_PASSWORD: Server_handle_option_update_password(client_socket_fd, user); break;

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

    if (pthread_mutex_init(&users_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }
    // this append does not need a mutex
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

    int opt = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

int Server_accept_connection(void) {
    int client_socket_fd = accept(server_socket_fd, NULL, NULL);
    if (client_socket_fd == -1) {
        perror("accept");
        exit(1);
    }
    return client_socket_fd;
}


static void* _Server_handle_client_thread(void* arg) {
    int client_socket_fd = *(int*)arg;
    free(arg);                     // free memory allocated for the fd

    Server_handle_client_sync(client_socket_fd);
    close(client_socket_fd);

    return NULL;
}

void Server_handle_client_async(int client_socket_fd) {
    pthread_t thread_id;

    // Pass the socket FD by pointer because the original variable may change
    int* fd_ptr = malloc(sizeof(int));
    if (!fd_ptr) {
        perror("malloc");
        close(client_socket_fd);
        return;
    }
    *fd_ptr = client_socket_fd;

    if (pthread_create(&thread_id, NULL, _Server_handle_client_thread, fd_ptr) != 0) {
        perror("pthread_create");
        close(client_socket_fd);
        free(fd_ptr);
        return;
    }

    // Optional: detach so thread cleans itself when finished
    pthread_detach(thread_id);
}
