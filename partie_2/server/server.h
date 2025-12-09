#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include "../shared.h"

extern pthread_mutex_t users_mutex;

void Server_init(void);
void Server_destroy(void);
int Server_accept_connection(void);
void Server_handle_client_async(int client_socket_fd);

#endif // SERVER_H