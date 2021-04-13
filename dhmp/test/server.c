#include <stdio.h>

#include "dhmp_server.h"

int main(int argc, char *argv[]) {
    int id;
    if (argc < 2) {
        printf("input param error. input:<filname> <id>\n");
        return -1;
    } else {
        id = atoi(argv[1]);
    }

    dhmp_server_init(id);
    dhmp_server_destroy();
    return 0;
}
