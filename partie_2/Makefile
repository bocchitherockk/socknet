.PHONY: compile clean tests gui

CC = gcc
CFLAGS = -Wall -Wextra -g

SHARED_HEADER = ./shared.h
SHARED_SOURCE = ./shared.c

SERVER_HEADER = ./server/server.h
SERVER_SOURCE = ./server/server.c ./server/User.c ./server/Users.c ./server/server_main.c
SERVER_EXECUTABLE = ./server/server_main

CLIENT_HEADER = ./client/client.h
CLIENT_SOURCE = ./client/client.c ./client/client_main.c
CLIENT_EXECUTABLE = ./client/client_main


compile: $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE)

$(SERVER_EXECUTABLE): Makefile $(SHARED_HEADER) $(SHARED_SOURCE) $(SERVER_HEADER) $(SERVER_SOURCE)
	$(CC) $(CFLAGS) -o $(SERVER_EXECUTABLE) $(SHARED_SOURCE) $(SERVER_SOURCE)

$(CLIENT_EXECUTABLE): Makefile $(SHARED_HEADER) $(SHARED_SOURCE) $(CLIENT_HEADER) $(CLIENT_SOURCE)
	$(CC) $(CFLAGS) -o $(CLIENT_EXECUTABLE) $(SHARED_SOURCE) $(CLIENT_SOURCE)

# tests
TEST_DIR = ./tests
TEST_SOURCE := $(wildcard $(TEST_DIR)/*.c)
TEST_EXECUTABLE := $(basename $(TEST_SOURCE))

%.test: %.test.c
	$(CC) $(CFLAGS) -o $@ $<

tests: compile $(TEST_EXECUTABLE)

clean:
	rm -f $(SERVER_EXECUTABLE)
	rm -f $(CLIENT_EXECUTABLE)
