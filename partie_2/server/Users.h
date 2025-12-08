#ifndef USERS_H
#define USERS_H

#include <stdlib.h>
#include "./User.h"

typedef struct Users {
    User *items;
    size_t count;
    size_t capacity;
} Users;


size_t Users_append(Users *users, User item);
void Users_remove_at(Users *users, size_t index);
User *Users_find_user_by_username(Users *users, char *username);


#endif // USERS_H