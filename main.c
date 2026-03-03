#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include "server.h"

#define PORT "8080"

void processArgs(char  *port, char *ipstr, int argc, char **argv) {
    if (argc >= 2) {
        strcpy(port, argv[1]);
    }
    if (argc >= 3) {
        strcpy(ipstr, argv[2]);
    }
    if (argc >= 4) {
        fprintf(stderr, "usage: http-test [port [address]]\n");
        exit(0);
    }
}

int main(int argc, char **argv) {
    char port[5] = PORT;
    char ipstr[INET6_ADDRSTRLEN] = "0.0.0.0";
    processArgs(port, ipstr, argc, argv);
    listen_on(ipstr, port);
    return 0;
}

