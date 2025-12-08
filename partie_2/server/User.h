#ifndef USER_H
#define USER_H

#include <stdbool.h>
#include <time.h>

typedef enum User_Role {
    USER_ROLE_ADMIN = 1,
    USER_ROLE_USER,
} User_Role;

typedef struct User {
    char username[256];
    char password[256];
    bool is_logged_in;
    time_t login_time;
    User_Role role;
} User;


User User_create(char *username, char *password, User_Role role);
bool User_login(User *user, char *password);
bool User_update_password(User *user, char *old_password, char *new_password);

#endif // USER_H