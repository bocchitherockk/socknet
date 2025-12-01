.PHONY: compile clean tests gui

CC = gcc
CFLAGS = -Wall -Wextra -g

SHARED_HEADER = ./shared.h
SHARED_SOURCE = ./shared.c

SERVER_HEADER = ./server/server.h
SERVER_SOURCE = ./server/server.c ./server/server_main.c
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

# gui
RAYLIB_INCLUDE = ./gui/raylib-5.5_linux_amd64/include
RAYLIB_LIB = ./gui/raylib-5.5_linux_amd64/lib
GUI_SOURCE = ./gui/gui.c ./gui/Process.c
GUI_EXECUTABLE = ./gui/gui
$(GUI_EXECUTABLE): compile $(GUI_SOURCE)
	$(CC) $(CFLAGS) -I$(RAYLIB_INCLUDE) -o $(GUI_EXECUTABLE) $(GUI_SOURCE) -L$(RAYLIB_LIB) -l:libraylib.a -lm

gui: $(GUI_EXECUTABLE)

clean:
	rm -f $(SERVER_EXECUTABLE)
	rm -f $(CLIENT_EXECUTABLE)
# 	rm -f $(TEST_EXECUTABLE)
# 	rm -f $(GUI_EXECUTABLE)
