#include "audio.h"

void retro_core_audio_sample(int16_t left, int16_t right) {
    int16_t buf[2] = {left, right};
	if (core_handler.counter_connections > 0) {
        for (int i=0;i<MAX_CONN;i++){
            pthread_mutex_lock(&core_handler.connections_mutex[i]);
            if (core_handler.connections[i] != NULL) {
                send_data(CMD_SEND_AUDIO, buf, sizeof(int16_t) * NUM_AUDIO_CHANNELS, core_handler.connections[i]);
            }
            pthread_mutex_unlock(&core_handler.connections_mutex[i]);
        }
    }
}

size_t retro_core_audio_sample_batch(const int16_t *data, size_t frames) {
    unsigned buff_size = sizeof(int16_t) * NUM_AUDIO_CHANNELS * frames;
    if (core_handler.counter_connections > 0) {
        for (int i=0;i<MAX_CONN;i++){
            pthread_mutex_lock(&core_handler.connections_mutex[i]);
            if (core_handler.connections[i] != NULL) {
                send_data(CMD_SEND_AUDIO, data, buff_size, core_handler.connections[i]);
            }
            pthread_mutex_unlock(&core_handler.connections_mutex[i]);
        }
    }
    return frames;
}
