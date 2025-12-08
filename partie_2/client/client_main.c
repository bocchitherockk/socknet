#include <stdio.h> // printf

#include "../shared.h"
#include "client.h"


int main(void) {
    Client_init();
    Client_connect();
    Client_run();
    Client_destroy();
   return 0;
}