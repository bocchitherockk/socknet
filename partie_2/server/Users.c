#include <stdlib.h>
#include <string.h>
#include "./Users.h"

size_t Users_append(Users *users, User item) {
    if (users->capacity == 0) { // not yet initialized
        users->capacity = 4;
        users->count = 0;
        users->items = malloc(users->capacity * sizeof(User));
    } else if (users->count == users->capacity) { // list is full
        users->capacity *= 2;
        User *new_list = malloc(users->capacity * sizeof(User));
        memcpy(new_list, users->items, users->count * sizeof(User));
        free(users->items);
        users->items = new_list;
    }
    users->items[users->count++] = item;
    return users->count - 1;
}

void Users_remove_at(Users *users, size_t index) {
    memmove(
        users->items + index*sizeof(User),
        users->items + (index+1)*sizeof(User),
        (users->count - (index+1)) * sizeof(User)
    );
    users->count--;
}

User *Users_find_user_by_username(Users *users, char *username) {
    for (size_t i = 0; i < users->count; i++) {
        User *user = &users->items[i];
        if (strcmp(user->username, username) == 0) return user;
    }
    return NULL;
}

