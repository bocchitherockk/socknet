#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../shared.h"
#include "./server.h"


static int server_socket_fd = 0;

struct sockaddr_in client_address = {0};
socklen_t client_address_length = sizeof(client_address);

void Server_init() {
    printf("Server running ...\n");
    srand(time(NULL) ^ getpid());

    server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_address = {
        .sin_addr = {0},
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Error: unable to parse SERVER_IP_ADDRESS '%s'\n", SERVER_IP_ADDRESS);
        exit(1);
    }

    // bind socket to port
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("bind");
        exit(1);
    }
}

void Server_destroy() {
    if (close(server_socket_fd) == -1) {
        perror("close");
        exit(1);
    }
    printf("Server stopped\n");
}

Question Server_read_question() {
    Question question = {0};
    if (recvfrom(server_socket_fd, &question.question, sizeof(question.question), 0, (struct sockaddr *) &client_address, &client_address_length) == -1) {
        perror("recvfrom");
        exit(1);
    }
    return question;
}

Answer Server_generate_answer(Question question) {
    Answer answer = {
        .count = question.question,
        .data = malloc(question.question * sizeof(int)),
    };

    if (answer.data == NULL) {
        perror("Server_generate_answer : malloc");
        exit(1);
    }
    for (size_t i = 0; i < question.question; i++) {
        answer.data[i] = rand() % ANSWER_SEED;
    }

    return answer;
}

void Server_write_answer(Answer answer) {
    if (sendto(server_socket_fd, &answer.count, sizeof(answer.count), 0, (struct sockaddr *) &client_address, client_address_length) == -1) {
        perror("sendto");
        exit(1);
    }
    if (sendto(server_socket_fd, answer.data, answer.count * sizeof(*answer.data), 0, (struct sockaddr *) &client_address, client_address_length) == -1) {
        perror("sendto");
        exit(1);
    }
}
