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
		case CMD_RECV_PLAY:
			g_retro.change_state_emulation(false);
			break;
		case CMD_RECV_PAUSE:
			g_retro.change_state_emulation(true);
			break;
		default:
			break;
	}
	return 0;
}

bool send_data(unsigned short int cmd, void * data, int size, int* conn) {
	int written_bytes = 0;
    if (conn != NULL){
		written_bytes = write(*conn, &cmd, sizeof(cmd));
		if (written_bytes <= 0) {
			return false;
		}
		write(*conn, &size, sizeof(size));
		if (written_bytes <= 0) {
			return false;
		}
		write(*conn, data, size);
		if (written_bytes <= 0) {
			return false;
		}
	}

    return true;
}