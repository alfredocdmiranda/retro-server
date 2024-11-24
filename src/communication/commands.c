#include "commands.h"

int read_command(unsigned short int cmd, void * data, int size, int player) {
	unsigned short key;
	switch (cmd){
		case CMD_RECV_EXIT:
			// TODO Improve the way it exits?
			exit(0);
			break;
		case CMD_RECV_KEY_PRESSED:
			key = *((unsigned short *)data);
			g_retro.joypads[player][key] = 1;
			break;
		case CMD_RECV_KEY_RELEASED:
			key = *((unsigned short *)data);
			g_retro.joypads[player][key] = 0;
			break;
		case CMD_RECV_RESET:
			g_retro.retro_reset();
			break;
		default:
			break;
	}
}

int send_data(unsigned short int cmd, void * data, int size, int* conn) {
    if (conn != NULL){
		write(*conn, &cmd, sizeof(cmd));
		write(*conn, &size, sizeof(size));
		write(*conn, data, size);
	}

    return 0;
}