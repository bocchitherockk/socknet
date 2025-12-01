#ifndef CLIENT_H
#define CLIENT_H


#include "../shared.h"

void Client_init();
void Client_destroy();
Question Client_generate_question();
void Client_write_question(Question question);
Answer Client_read_answer();

#endif // CLIENT_H