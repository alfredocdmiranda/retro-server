#ifndef COMMUNICATION_COMMANDS_H
#define COMMUNICATION_COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../emulation.h"

#define CMD_SEND_AUDIO 1
#define CMD_SEND_VIDEO 2
#define CMD_SEND_AV_INFO 3

#define CMD_RECV_EXIT 0
#define CMD_RECV_KEY_PRESSED 1
#define CMD_RECV_KEY_RELEASED 2
#define CMD_RECV_RESET 3
#define CMD_RECV_PLAY 4
#define CMD_RECV_PAUSE 5

int send_data(unsigned short int cmd, void * data, int size, int* conn);
int read_command(unsigned short int cmd, void * data, int size, int player);

#endif