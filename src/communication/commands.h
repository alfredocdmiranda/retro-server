#ifndef COMMUNICATION_COMMANDS_H
#define COMMUNICATION_COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CMD_SEND_AUDIO 1
#define CMD_SEND_VIDEO 2
#define CMD_SEND_AV_INFO 3

int send_data(unsigned short int cmd, void * data, int size, int* conn);

#endif