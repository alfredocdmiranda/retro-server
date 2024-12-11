#include "core.h"

RetroHandler core_handler = {0};

void change_state_emulation(bool paused) {
    core_handler.paused = paused;

    // for (int i=0;i<MAX_CONN;i++){
    //     pthread_mutex_lock(&core_handler.connections_mutex[i]);
    //     if (core_handler.connections[i] != NULL) {
    //         // Broadcast state data. Use config?
    //         // send_data(CMD_SEND_AUDIO, data, buff_size, core_handler.connections[i]);
    //     }
    //     pthread_mutex_unlock(&core_handler.connections_mutex[i]);
    // }
}

/**
 * Logs a message to the frontend.
 *
 * @param level The log level of the message.
 * @param fmt The format string to log.
 * Same format as \c printf.
 * Behavior is undefined if this is \c NULL.
 * @param ... Zero or more arguments used by the format string.
 * Behavior is undefined if these don't match the ones expected by \c fmt.
 */
static void retro_core_log(enum retro_log_level level, const char *format, ...) {
    // TODO: This function should be using log_message from utils.c.
    if (level >= LOG_LEVEL) {
        const char *level_str;
        switch (level) {
            case RETRO_LOG_DEBUG: 
                level_str = "DEBUG"; 
                break;
            case RETRO_LOG_INFO: 
                level_str = "INFO"; 
                break;
            case RETRO_LOG_WARN: 
                level_str = "WARNING"; 
                break;
            case RETRO_LOG_ERROR: 
                level_str = "ERROR"; 
                break;
            default: 
                level_str = "UNKNOWN"; 
                break;
        }

        // Get the current timestamp
        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));

        // Print the log level and the message
        printf("[%s] [%s] [CORE] ", timestamp, level_str);

        // Handle variable arguments
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        // Print a newline at the end
        printf("\n");
    }
   
    if (level == RETRO_LOG_ERROR) {
        exit(EXIT_FAILURE);
    }
}

/**
 * Environment callback to give implementations a way of performing uncommon tasks.
 *
 * @param cmd The command to run.
 * @param data A pointer to the data associated with the command.
 *
 * @return Varies by callback, but will always return \c false if the command is not recognized.
 */
static bool retro_core_environment(unsigned cmd, void *data) {
    bool result = true;
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            struct retro_log_callback *cb = (struct retro_log_callback *)data;
            cb->log = retro_core_log;
            break;
        }
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: {
            bool *bval = (bool *)data;
            *bval = true;
            break;
        }
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
            result = video_set_pixel_format((unsigned int)*fmt);
            break;
        }
        case RETRO_ENVIRONMENT_SET_HW_RENDER: {
            struct retro_hw_render_callback *hw_render = (struct retro_hw_render_callback*)data;
            hw_render->get_current_framebuffer = retro_core_get_current_framebuffer;
            hw_render->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
            video_info.hw_render = *hw_render;
            break;
        }
        case RETRO_ENVIRONMENT_SET_GEOMETRY: {
            result = video_set_geometry((const struct retro_game_geometry *)data);
            break;
        }
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
            // TODO Need to improve this
            *(const char **)data = ".";
            break;
        }
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
            // TODO Need to improve this
            *(const char **)data = ".";
            break;
        }
        default: {
            retro_core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
            result = false;
        }
    }

    return result;
}

int load_core(const char *sofile) {
    // Local functions to set the callbacks
    void (*set_environment)(retro_environment_t) = NULL;
    void (*set_video_refresh)(retro_video_refresh_t) = NULL;
    void (*set_input_poll)(retro_input_poll_t) = NULL;
    void (*set_input_state)(retro_input_state_t) = NULL;
    void (*set_audio_sample)(retro_audio_sample_t) = NULL;
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
    
    core_handler.so = dlopen(sofile, RTLD_LAZY);
    if (!core_handler.so){
        log_message(LOG_LEVEL_ERROR, "Error loading library: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    // Link to functions in core file
    load_sym(core_handler.retro_init, retro_init, core_handler.so);
    load_sym(core_handler.retro_deinit, retro_deinit, core_handler.so);
    load_sym(core_handler.retro_api_version, retro_api_version, core_handler.so);
    load_sym(core_handler.retro_get_system_info, retro_get_system_info, core_handler.so);
    load_sym(core_handler.retro_get_system_av_info, retro_get_system_av_info, core_handler.so);
    load_sym(core_handler.retro_set_controller_port_device, retro_set_controller_port_device, core_handler.so);
    load_sym(core_handler.retro_load_game, retro_load_game, core_handler.so);
    load_sym(core_handler.retro_load_game_special, retro_load_game_special, core_handler.so);
    load_sym(core_handler.retro_unload_game, retro_unload_game, core_handler.so);
    load_sym(core_handler.retro_serialize_size, retro_serialize_size, core_handler.so);
    load_sym(core_handler.retro_serialize, retro_serialize, core_handler.so);
    load_sym(core_handler.retro_unserialize, retro_unserialize, core_handler.so);
    load_sym(core_handler.retro_reset, retro_reset, core_handler.so);
    load_sym(core_handler.retro_run, retro_run, core_handler.so);
    load_sym(core_handler.retro_get_region, retro_get_region, core_handler.so);
    load_sym(core_handler.retro_get_memory_data, retro_get_memory_data, core_handler.so);
    load_sym(core_handler.retro_get_memory_size, retro_get_memory_size, core_handler.so);
    load_sym(core_handler.retro_cheat_reset, retro_cheat_reset, core_handler.so);
    load_sym(core_handler.retro_cheat_set, retro_cheat_set, core_handler.so);
    
    load_sym(set_environment, retro_set_environment, core_handler.so);
    load_sym(set_video_refresh, retro_set_video_refresh, core_handler.so);
    load_sym(set_input_poll, retro_set_input_poll, core_handler.so);
    load_sym(set_input_state, retro_set_input_state, core_handler.so);
    load_sym(set_audio_sample, retro_set_audio_sample, core_handler.so);
    load_sym(set_audio_sample_batch, retro_set_audio_sample_batch, core_handler.so);
    
    // Set callbacks
    set_environment(retro_core_environment);
    set_video_refresh(retro_core_video_refresh);
    set_input_poll(retro_core_input_poll);
    set_input_state(retro_core_input_state);
    set_audio_sample(retro_core_audio_sample);
    set_audio_sample_batch(retro_core_audio_sample_batch);

    core_handler.change_state_emulation = *change_state_emulation;

    core_handler.retro_init();
    core_handler.initialized = true;
    core_handler.paused = false;

    return EXIT_SUCCESS;
}

int load_game_from_file(const char *filename) {
    if (!filename) {
        log_message(LOG_LEVEL_ERROR, "Error: Filename is NULL.\n");
        return EXIT_FAILURE;
    }

    struct retro_system_info system = {0};
    struct retro_game_info info = {filename, NULL, 0, NULL};
    FILE *file = fopen(filename, "rb");
    void *game_data = NULL;

    if (!file) {
        log_message(LOG_LEVEL_ERROR, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    info.size = ftell(file);
    if (info.size == -1L) {
        log_message(LOG_LEVEL_ERROR, "Failed to determine file size for '%s': %s\n", filename, strerror(errno));
        cleanup(file, NULL);
        return EXIT_FAILURE;
    }
    rewind(file);

    // Get system information
    core_handler.retro_get_system_info(&system);

    // Load game data into memory if needed
    if (!system.need_fullpath) {
        game_data = malloc(info.size);
        if (!game_data) {
            log_message(LOG_LEVEL_ERROR, "Memory allocation failed.\n");
            cleanup(file, NULL);
            return EXIT_FAILURE;
        }

        if (fread(game_data, info.size, 1, file) != 1) {
            log_message(LOG_LEVEL_ERROR, "Failed to read game data from '%s'.\n", filename);
            cleanup(file, game_data);
            return EXIT_FAILURE;
        }
        info.data = game_data;
    }

    // Load game into the emulator
    if (!core_handler.retro_load_game(&info)) {
        log_message(LOG_LEVEL_ERROR, "Failed to load game '%s' into the emulator.\n", filename);
        cleanup(file, game_data);
        return EXIT_FAILURE;
    }
    
    cleanup(file, game_data);

    struct retro_system_av_info av = {0};
    core_handler.retro_get_system_av_info(&av);
    log_message(LOG_LEVEL_DEBUG, "Aspect Ratio: %f", av.geometry.aspect_ratio);
    log_message(LOG_LEVEL_DEBUG, "Height: %d | Width: %d", av.geometry.base_height, av.geometry.base_width);
    log_message(LOG_LEVEL_DEBUG, "Game loaded");

    video_init(&av.geometry);

    return EXIT_SUCCESS;
}