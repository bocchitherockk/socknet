#include <stdio.h> // printf
#include <stdbool.h> // true

#include "../shared.h"
#include "./server.h"

void print_read_question(Question question) {
    printf("server: read question: { question = %zu, client_address = %d.%d.%d.%d, client_port = %d }\n",
        question.question,
        ((client_address.sin_addr.s_addr) >> (8*3)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*2)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*1)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*0)) & 0xff,
        client_address.sin_port
    );
}

void print_generated_answer(Answer answer) {
    printf("server: generated answer: { count = %zu, answers: ", answer.count);
    for (size_t i = 0; i < answer.count; i++) {
        printf("%d ", answer.data[i]);
    }
    printf(", client_address = %d.%d.%d.%d, client_port = %d }\n",
        ((client_address.sin_addr.s_addr) >> (8*3)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*2)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*1)) & 0xff,
        ((client_address.sin_addr.s_addr) >> (8*0)) & 0xff,
        client_address.sin_port
    );
}

Question question = {0};
Answer answer = {0};

int main(void) {
    Server_init();

    while (true) {
        question = Server_read_question();
        print_read_question(question);
        answer = Server_generate_answer(question);
        print_generated_answer(answer);
        Server_write_answer(answer);
        Answer_destroy(&answer);
    }

    Server_destroy();
   return 0;
}