#ifndef SERVER_H
#define SERVER_H


#include "../shared.h"

// to hold the current client info
extern struct sockaddr_in client_address;
extern socklen_t client_address_length;

void Server_init();
void Server_destroy();
Question Server_read_question();
Answer Server_generate_answer(Question question);
void Server_write_answer(Answer answer);

#endif // SERVER_H