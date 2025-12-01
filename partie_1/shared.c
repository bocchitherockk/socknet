#include <stdlib.h> // free

#include "./shared.h"

void Answer_destroy(Answer *answer) {
    if (answer->data != NULL) free(answer->data);
    answer->count = 0;
    answer->data = NULL;
}