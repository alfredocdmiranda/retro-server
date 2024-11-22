#include "commands.h"

int send_data(unsigned short int cmd, void * data, int size, int* conn) {
    if (conn != NULL){
		write(*conn, &cmd, sizeof(cmd));
		write(*conn, &size, sizeof(size));
		write(*conn, data, size);
	}

    return 0;
}