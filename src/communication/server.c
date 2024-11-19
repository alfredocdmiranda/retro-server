#include "server.h"

void create_server(int *server_socket, struct sockaddr_in *server_addr, uint16_t port) {
    (*server_socket) = socket(AF_INET, SOCK_STREAM, 0);
    if ((*server_socket) == -1) {
        log_message(LOG_LEVEL_ERROR, "%s[%d]", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt((*server_socket), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message(LOG_LEVEL_ERROR, "Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr->sin_port = htons(port);

    if ((bind((*server_socket), (struct sockaddr *)server_addr, sizeof(*server_addr))) < 0) {
        log_message(LOG_LEVEL_ERROR, "%s[%d]\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    if ((listen((*server_socket), MAX_CONN)) != 0) {
        log_message(LOG_LEVEL_ERROR, "%s[%d]\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
}

int wait_connection(struct sockaddr_in *server_addr, int *server_socket, int *client_socket) {
    socklen_t addr_len = sizeof(*server_addr);
    *client_socket = accept(*server_socket, (struct sockaddr *)server_addr, &addr_len);
    if (client_socket < 0) {
        log_message(LOG_LEVEL_ERROR, "Server accept failed...\n");
        return -1;
    }

    return 0;
}