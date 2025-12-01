#include <stdio.h> // sprintf, fclose, perror
#include <stdlib.h> // free, size_t, exit
#include <stdbool.h> // true, false
#include <string.h> // strlen
#include <unistd.h> // STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO, execlp, close, fork, pipe, dup2
#include <pthread.h> // pthread_t, pthread_create
#include <signal.h> // kill, SIGKILL

#include "./Process.h"
#include "./Process_utils.c"
#include "./Processes_utils.c"



Process server = {0};
Processes clients = {0};

void *_Process_create_routine(void *Process_Type__process_type) {
    Process_Type process_type = *(Process_Type*)Process_Type__process_type;
    free(Process_Type__process_type);
    if (process_type == PROCESS_TYPE_SERVER && server.is_set) {
        return NULL;
    }

    Process *new_process_reference = NULL;
    Process new_process = {
        .is_set = true,
        .process_type = process_type,
        .process_state = PROCESS_STATE_EXECUTING,
    };

    if (process_type == PROCESS_TYPE_SERVER) {
        server = new_process;
        new_process_reference = &server;
    } else if (process_type == PROCESS_TYPE_CLIENT) {
        size_t index_of_insertion = Processes_append(&clients, new_process);
        new_process_reference = &clients.items[index_of_insertion];
    }

    if (pipe(new_process_reference->write_pipe) == -1) {
        perror("pipe write_pipe");
        exit(1);
    }
    if (pipe(new_process_reference->read_pipe) == -1) {
        perror("pipe read_pipe");
        exit(1);
    }

    int gdb_process = fork();
    if (gdb_process < 0) {
        perror("fork");
        exit(1);
    }

    if (gdb_process == 0) { // Child
        if (dup2(new_process_reference->write_pipe[0], STDIN_FILENO) == -1) { perror("dup2 stdin");  exit(1); }
        if (dup2(new_process_reference->read_pipe[1], STDOUT_FILENO) == -1) { perror("dup2 stdout"); exit(1); }
        if (dup2(new_process_reference->read_pipe[1], STDERR_FILENO) == -1) { perror("dup2 stderr"); exit(1); }

        // Close unused fds in child
        close(new_process_reference->write_pipe[0]);
        close(new_process_reference->write_pipe[1]);
        close(new_process_reference->read_pipe[0]);
        close(new_process_reference->read_pipe[1]);

        execlp("gdb", "gdb", "--interpreter=mi2", NULL);
        perror("execvp gdb");
        exit(127);
    }

    // Parent
    // Close unused ends
    close(new_process_reference->write_pipe[0]);
    close(new_process_reference->read_pipe[1]);

    // Wrap the writable end with FILE* for easy fprintf
    new_process_reference->to_gdb = fdopen(new_process_reference->write_pipe[1], "w");
    if (!new_process_reference->to_gdb) {
        perror("fdopen to_gdb");
        close(new_process_reference->write_pipe[1]);
        close(new_process_reference->read_pipe[0]);
        exit(1);
    }

    char *executable = process_type == PROCESS_TYPE_SERVER ? "./server/server_main" : "./client/client_main";
    new_process_reference->pid = gdb_process;
    new_process_reference->message     = calloc(MESSAGE_BUFFER_SIZE, sizeof(char));
    new_process_reference->source_code = calloc(SOURCE_CODE_BUFFER_SIZE, sizeof(char));
    new_process_reference->answer_data = calloc(ANSWER_DATA_BUFFER_SIZE, sizeof(char));
    sprintf(new_process_reference->source_file, "%s.c", executable);
    read_entire_file(new_process_reference->source_file, new_process_reference->source_code);

    // drain gdb startup banner
    read_message(new_process_reference->read_pipe, new_process_reference->message);

    send_commandf(new_process_reference->to_gdb, "-file-exec-and-symbols %s", executable);
    read_message(new_process_reference->read_pipe, new_process_reference->message);

    send_command(new_process_reference->to_gdb, "-interpreter-exec console \"handle SIGUSR1 nostop noprint pass\"");
    read_message(new_process_reference->read_pipe, new_process_reference->message);

    send_command(new_process_reference->to_gdb, "-break-insert main");
    read_message(new_process_reference->read_pipe, new_process_reference->message);

    send_command(new_process_reference->to_gdb, "-exec-run"); // this blocks at the fist line in main
    read_message(new_process_reference->read_pipe, new_process_reference->message);

    extract_property_from_message(new_process_reference->message, "line=\"", new_process_reference->line_number); // this will be displayed in the gui, but not only, primarily it is needed to extract the line in the source code
    get_line_content(new_process_reference->source_code, atoi(new_process_reference->line_number), new_process_reference->line_content); // this will be displayed in the gui

    new_process_reference->process_state = PROCESS_STATE_STOPPED;
    return NULL;
}

void _Process_destroy(Process *process) {
    send_command(process->to_gdb, "-gdb-exit");
    // read_message(process->read_pipe, process->message); // this is not needed honestly, who cares about the gdb message when we are exiting, we are not using it anywhere anyway

    // Cleanup
    process->process_state = PROCESS_STATE_ENDED;
    fclose(process->to_gdb); // i think this closes write_pipe[1] automatically
    close(process->read_pipe[0]);
    free(process->message);
    free(process->source_code);
    if (process->process_type == PROCESS_TYPE_SERVER) {
        server = (Process){.is_set = false};
    } else if (process->process_type == PROCESS_TYPE_CLIENT) {
        Processes_remove_by_pid(&clients, process->pid);
    }
}

void *_Process_next_routine(void *Process_ptr__process) {
    Process *process = (Process *)Process_ptr__process;
    if (process->process_state == PROCESS_STATE_EXECUTING) return NULL; // note: for now probably this function is never called for a PROCESS_STATE_ENDED process, but be careful if in the future you want to change it and keep displaying ended processes with a special style (meaning the button "next" will still be there)

    process->process_state = PROCESS_STATE_EXECUTING;
    send_command(process->to_gdb, "-exec-next");
    read_message_blocking(process->read_pipe, process->message);

    if (program_has_ended(process->message)) {
        _Process_destroy(process);
        return NULL;
    }

    extract_property_from_message(process->message, "line=\"", process->line_number);
    get_line_content(process->source_code, atoi(process->line_number), process->line_content);

    send_command(process->to_gdb, "-data-evaluate-expression question"); // this will be displayed in the gui
    read_message(process->read_pipe, process->message);
    extract_property_from_message(process->message, "value=\"", process->question);

    send_command(process->to_gdb, "-data-evaluate-expression answer"); // this will be displayed in the gui
    read_message(process->read_pipe, process->message);
    extract_property_from_message(process->message, "value=\"", process->answer);

    Answer answer = parse_answer(process->answer);
    if (answer.data != NULL) {
        send_commandf(process->to_gdb, "-data-evaluate-expression (*answer.data)@%d", answer.count); // this will be displayed in the gui
        read_message(process->read_pipe, process->message);
        extract_property_from_message(process->message, "value=\"", process->answer_data);
        process->answer_data[0] = '[';
        process->answer_data[strlen(process->answer_data) - 1] = ']';
    }

    process->process_state = PROCESS_STATE_STOPPED;
    return NULL;
}

void Process_create(Process_Type process_type) {
    pthread_t thread;
    Process_Type *arg = malloc(sizeof(Process_Type));
    *arg = process_type;
    if (pthread_create(&thread, NULL, _Process_create_routine, arg) != 0) {
        perror("pthread_create");
        exit(1);
    }
}

void Process_next(Process *process) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, _Process_next_routine, process) != 0) {
        perror("pthread_create");
        exit(1);
    }
}

void Process_kill_all() {
    if (kill(server.pid, SIGKILL) == -1) {
        perror("kill");
        exit(1);
    }

    for (size_t i = 0; i < clients.count; i++) {
        if (kill(clients.items[i].pid, SIGKILL) == -1) {
            perror("kill");
            exit(1);
        }
    }
}