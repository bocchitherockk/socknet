#include <stdio.h> // perror
#include <stdlib.h> // exit, malloc
#include <unistd.h> // write, read, getpid, pause, close, access
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../shared.h"
#include "./client.h"


static int client_socket_fd = 0;


void Client_init() {
    srand(time(NULL) ^ getpid());
    if ((client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
}

void Client_destroy() {
    if (close(client_socket_fd) == -1) {
        perror("close");
        exit(1);
    }
}

Question Client_generate_question() {
    return (Question){
        .question = (rand() % QUESTIONS_SEED) + 1,
    };
}

void Client_write_question(Question question) {
    struct sockaddr_in server_address = {
        .sin_addr = {0},
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
    };
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Error: unable to parse SERVER_IP_ADDRESS '%s'\n", SERVER_IP_ADDRESS);
        exit(1);
    }
    if (sendto(client_socket_fd, &question.question, sizeof(question.question), 0, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("sendto");
        exit(1);
    }
}

Answer Client_read_answer() {
    Answer answer = {0};

    if (recvfrom(client_socket_fd, &answer.count, sizeof(answer.count), 0, NULL, NULL) == -1) {
        perror("recvfrom");
        exit(1);
    }

    answer.data = malloc(answer.count * sizeof(int));
    if (answer.data == NULL) {
        perror("Client_read_answer : malloc");
        exit(1);
    }

    if (recvfrom(client_socket_fd, answer.data, answer.count * sizeof(int), 0, NULL, NULL) == -1) {
        perror("recvfrom");
        exit(1);
    }

    return answer;
}