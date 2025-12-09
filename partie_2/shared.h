#ifndef SHARED_H
#define SHARED_H

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5000

#include <sys/socket.h>
#include <netinet/in.h>

typedef enum Command {
    COMMAND_DISPLAY = 1,  // Just display
    COMMAND_INPUT_STRING, // Display and wait for string input
    COMMAND_INPUT_SIZE_T, // Display and wait for size_t input
    COMMAND_INPUT_INT,    // Display and wait for integer input
    COMMAND_DISCONNECT    // Close connection
} Command;

void Shared_send_int(int socket_fd, int x);
int Shared_read_int(int socket_fd);

void Shared_send_string(int socket_fd, char *string);
char *Shared_read_string(int socket_fd);

#endif // SHARED_H