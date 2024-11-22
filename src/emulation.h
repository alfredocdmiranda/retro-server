#ifndef EMULATION_H
#define EMULATION_H

#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communication/commands.h"
#include "constants.h"
#include "libretro.h"
#include "utils.h"


#define NUM_JOYPADS 2
#define NUM_BUTTONS RETRO_DEVICE_ID_JOYPAD_R3 + 1
#define NUM_AUDIO_CHANNELS 2

// Load core's symbols to local variables to be called
#define load_sym(VAR, SYMBOL, SO_LIB)                     \
    do {                                                  \
        if (!((*(void **)&VAR) = dlsym(SO_LIB, #SYMBOL))) \
            exit(1);                                      \
    } while (0)

typedef struct {
    void *handle;
    bool initialized;
    int** connections;
    int* counter_connections;
    enum retro_pixel_format video_fmt;

    void (*retro_init)(void);
    void (*retro_deinit)(void);
    unsigned (*retro_api_version)(void);
    void (*retro_get_system_info)(struct retro_system_info *info);
    void (*retro_get_system_av_info)(struct retro_system_av_info *info);
    void (*retro_set_controller_port_device)(unsigned port, unsigned device);
    bool (*retro_load_game)(const struct retro_game_info *game);
    bool (*retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
    void (*retro_unload_game)(void);
    size_t (*retro_serialize_size)(void);
    bool (*retro_serialize)(void *data, size_t size);
    bool (*retro_unserialize)(const void *data, size_t size);
    void (*retro_reset)(void);
    void (*retro_run)(void);
    unsigned (*retro_get_region)(void);
    void (*retro_get_memory_data)(unsigned id);
    size_t (*retro_get_memory_size)(unsigned id);
    void (*retro_cheat_reset)(void);
    void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);
}  RetroHandler;
extern RetroHandler g_retro;

int load_core(const char *sofile);
int load_game_from_file(const char *filename);

#endif