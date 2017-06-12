#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"
#include "error.h"
#include "request.h"

extern bs_config config;

int sock_fd;

void term(int signo) {
    printf("received sigterm[%d], killing\n", signo);
    close(sock_fd);
    exit(0);
}

int main(int argc, char **argv) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    pthread_t thread;
    pthread_attr_t thread_attr;
    int bind_res;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    bs_config_parse_opts(argc, argv);

    signal(SIGTERM, term);
    signal(SIGINT, term);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0)
        bserve_fatal("opening socket failed");

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config.host_selected ? config.host : "127.0.0.1");
    server_addr.sin_port = htons(config.port_selected ? atoi(config.port) : 8080);

    bind_res = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (bind_res < 0)
        bserve_fatal("address binding failed");

    listen(sock_fd, 5);

    for (;;) {
        int *conn_fd = malloc(sizeof(int));

        if (conn_fd == NULL)
            bserve_fatal("unable to allocate memory");

        *conn_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_size);

        if (*conn_fd < 0)
            bserve_fatal("connection accept failed");

        pthread_create(&thread, &thread_attr, accept_request, conn_fd);
    }

    return 0;
}
