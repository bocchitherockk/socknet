#include <stdio.h> // printf
#include <stdbool.h> // true
#include <unistd.h>

#include "../shared.h"
#include "./server.h"


int main(void) {
    Server_init();
    while (true) {
        int client_socket_fd = Server_accept_connection();
        Server_handle_client_async(client_socket_fd);
    }
    Server_destroy();
   return 0;
}

// int main(void) {
//     Server_init();
//     while (true) Server_handle_client_async(Server_accept_conection());
//     Server_destroy();
//    return 0;
// }