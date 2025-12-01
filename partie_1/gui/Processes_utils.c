#include <stddef.h> // size_t
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

#include "./Process.h"

size_t Processes_append(Processes *processes, Process item) {
    if (processes->capacity == 0) { // not yet initialized
        processes->capacity = 4;
        processes->count = 0;
        processes->items = malloc(processes->capacity * sizeof(Process));
    } else if (processes->count == processes->capacity) { // list is full
        processes->capacity *= 2;
        Process *new_list = malloc(processes->capacity * sizeof(Process));
        memcpy(new_list, processes->items, processes->count * sizeof(Process));
        free(processes->items);
        processes->items = new_list;
    }
    processes->items[processes->count++] = item;
    return processes->count - 1;
}

void _Processes_remove_at(Processes *processes, size_t index) {
    memmove(
        processes->items + index*sizeof(Process),
        processes->items + (index+1)*sizeof(Process),
        (processes->count - (index+1)) * sizeof(Process)
    );
    processes->count--;
}

void Processes_remove_by_pid(Processes *processes, int pid) {
    for (size_t i = 0; i < processes->count; i++) {
        if (processes->items[i].pid == pid) {
            _Processes_remove_at(processes, i);
            break;
        }
    }
}