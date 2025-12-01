#include <stdio.h> // printf

#include "../shared.h"
#include "client.h"

void print_generated_question(Question question) {
    printf("client: asked question %zu\n", question.question);
}

void print_read_answer(Answer answer) {
    printf("client: read answer: { count = %zu, data = ", answer.count);
    for (size_t i = 0; i < answer.count; i++) {
        printf("%d ", answer.data[i]);
    }
    printf("}\n");
}

Question question = {0};
Answer answer = {0};

int main(void) {
    Client_init();

    question = Client_generate_question();
    print_generated_question(question);
    Client_write_question(question);
    answer = Client_read_answer();
    print_read_answer(answer);

    Answer_destroy(&answer);
    Client_destroy();
   return 0;
}