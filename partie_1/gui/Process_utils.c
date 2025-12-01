#include <stdio.h> // FILE, SEEK_END, SEEK_SET, ftell, fseek, fopen, fclose, fread, perror
#include <stdlib.h> // exit, abort
#include <string.h> // strstr, strlen
#include <stdbool.h> // bool, true
#include <stdarg.h> // va_start, va_end
#include <unistd.h> // read
#include <errno.h> // errno, EINTR
#include <sys/select.h> // fd_set, struct timeval, FD_ZERO, FD_SET, FD_ISSET, select

#include "../shared.h"

#define MESSAGE_BUFFER_SIZE     1024 * 1024     // 1 MB
#define SOURCE_CODE_BUFFER_SIZE 1024 * 1024 * 5 // 5 MB
#define ANSWER_DATA_BUFFER_SIZE 1024 * 1024     // 1 MB


void read_entire_file(char *filename, char *result) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fread(result, 1, filesize, file);
    result[filesize] = '\0';

    fclose(file);
}

void extract_property_from_message(char *message, char *property_pattern, char *result) { // in the format '$property="$value"'
    char *pos1 = strstr(message, property_pattern);
    char *pos2 = strstr(pos1 + strlen(property_pattern), "\"");
    int i = 0;
    char *c = pos1 + strlen(property_pattern);
    while (c != pos2) result[i++] = *c++;
}

// advances the pointer
// this is destructive, so in ccase it fails, i will abort the program
bool read_string(char **source, char *string) {
    while (**source != '\0' && *string != '\0' && **source == *string) {
        (*source)++;
        string++;
    }

    if (**source == '\0' && *string == '\0') return true;
    if (**source == '\0') return false;
    if (*string == '\0') return true;
    return false;
}

Answer parse_answer(char *answer) { // in the format '{count = 0, data = 0x0}'
    read_string(&answer, "{count = ");
    size_t count = strtol(answer, &answer, 10);
    read_string(&answer, ", data = ");
    void *data = (void *)strtol(answer, &answer, 16);
    read_string(&answer, "}");

    return (Answer){
        .count = count,
        .data = data,
    };
}

void get_line_content(char *file_content, int line_number, char *result) {
    int current_character = 0;
    int current_line = 1;

    // skip until the desired line
    while (current_line < line_number) {
        if (file_content[current_character]   == '\0') abort();
        if (file_content[current_character++] == '\n') current_line++;
    }

    if (
        file_content[current_character] == '\0' ||
        file_content[current_character] == '\n'
    ) { // means that this is an empty line
        result[0] = '\0';
    }

    int i = 0;
    while (true) {
        if (file_content[current_character + i] == '\n') break;
        if (file_content[current_character + i] == '\0') break;
        result[i] = file_content[current_character + i];
        i++;
    }
    result[i] = '\0';
}

bool program_has_ended(char *message) {
    // return strstr(message, "*stopped,reason=\"exited-normally\"") != NULL;
    return strstr(message, "func=\"__libc_start_call_main\"") != NULL;
}


// send a line (MI command) to gdb via the write FILE*
void send_command(FILE *to_gdb, const char *command) {
    fprintf(to_gdb, "%s\n", command);
    fflush(to_gdb);
}

void send_commandf(FILE *to_gdb, const char *fmt, ...) {
    char command[100];
    va_list args;
    va_start(args, fmt);
    vsprintf(command, fmt, args);
    va_end(args);

    send_command(to_gdb, command);
}


// read available data from fd until a timeout of TIMEOUT_MS without data
void read_message(int read_pipe[], char *message) {
    const int TIMEOUT_MS = 200;

    fd_set rfds;
    size_t total_read = 0;

    // convert ms to timeval
    struct timeval tv;
    tv.tv_sec = TIMEOUT_MS / 1000;
    tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;

    // We'll repeatedly select+read until select times out
    while (true) {
        FD_ZERO(&rfds);
        FD_SET(read_pipe[0], &rfds);
        struct timeval tv_local = tv;

        int rv = select(read_pipe[0]+1, &rfds, NULL, NULL, &tv_local);
        if (rv == -1) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        } else if (rv == 0) {
            // timeout, no more data
            break;
        }

        if (FD_ISSET(read_pipe[0], &rfds)) {
            size_t n = read(read_pipe[0], message + total_read, MESSAGE_BUFFER_SIZE - 1);
            if (n > 0) {
                message[n + total_read] = '\0';
                total_read += n;
                continue; // continue to try to read more available data
            } else if (n == 0) {
                // EOF (child likely exited)
                break;
            } else {
                if (errno == EINTR) continue;
                perror("read");
                break;
            }
        }
    }
}

// this function is called after sending a command that would trigger the execution of a blocking line in the execuable. for example open("fifo", O_RDONLY) would block until something opens the fifo for writing. this function will block until a complete message is received from gdb.
void read_message_blocking(int read_pipe[], char *message) {
    size_t total_read = 0;

    while (true) {
        size_t n = read(read_pipe[0], message + total_read, MESSAGE_BUFFER_SIZE - 1);
        if (n > 0) {
            total_read += n;
            message[total_read] = '\0';
            // Check if we have received a complete MI message (contains "*stopped")
            if (strstr(message, "*stopped")) { // means gdb has stopped at the next breakpoint (in other words, the command has completed)
                break;
            }
        } else if (n == 0) {
            // EOF
            break;
        } else {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }
    }
}