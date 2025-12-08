#ifndef SERVER_H
#define SERVER_H


#include "../shared.h"

void Server_init(void);
void Server_destroy(void);
int Server_accept_conection(void);
void Server_handle_client_async(int client_socket_fd);

#endif // SERVER_H