#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "./User.h"

User User_create(char *username, char *password, User_Role role) {
    User result = { .is_logged_in = false, .role = role, };
    strncpy(result.username, username, sizeof(result.username));
    strncpy(result.password, password, sizeof(result.password));
    return result;
}

bool User_login(User *user, char *password) {
    if (strcmp(user->password, password) != 0) return false;
    user->is_logged_in = true;
    user->login_time = time(NULL);
    return true;
}

bool User_update_password(User *user, char *old_password, char *new_password) {
    assert(new_password != NULL);
    if (strcmp(user->password, old_password) != 0) return false;
    strncpy(user->password, new_password, sizeof(user->password));
    return true;
}