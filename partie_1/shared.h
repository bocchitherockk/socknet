#ifndef SHARED_H
#define SHARED_H

#define QUESTIONS_SEED 10
#define ANSWER_SEED 1000

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 5000

#include <sys/socket.h>
#include <netinet/in.h>


typedef struct Question {
    size_t question;
} Question;

typedef struct Answer {
    size_t count;
    int *data;
} Answer;

void Answer_destroy(Answer *answer);


#endif // SHARED_H