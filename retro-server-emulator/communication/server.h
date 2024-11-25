#ifndef COMMUNICATION_SERVER_H
#define COMMUNICATION_SERVER_H

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../constants.h"
#include "../utils.h"

void create_server(int *sockfd, struct sockaddr_in *servAddr, uint16_t port);
int wait_connection(struct sockaddr_in *server_addr, int *server_socket, int *client_socket);

#endif