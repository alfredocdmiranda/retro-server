#include "emulation.h"

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

RetroHandler g_retro = {0};

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
    bool *bval;
    bool result = true;
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            struct retro_log_callback *cb = (struct retro_log_callback *)data;
            cb->log = retro_core_log;
            break;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            bval = (bool *)data;
            *bval = true;
            break;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            g_retro.video_fmt = *(enum retro_pixel_format*)data;

            if (g_retro.video_fmt > RETRO_PIXEL_FORMAT_RGB565){
                result = false;
            }
            break;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            // TODO Need to improve this
            *(const char **)data = ".";
            break;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            // TODO Need to improve this
            *(const char **)data = ".";
            break;
        default:
            retro_core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
            result = false;
    }

    return result;
}

/**
 * Render a frame.
 *
 * @param data A pointer to the frame buffer data with a pixel format of 15-bit \c 0RGB1555 native endian, unless changed with \c RETRO_ENVIRONMENT_SET_PIXEL_FORMAT.
 * @param width The width of the frame buffer, in pixels.
 * @param height The height frame buffer, in pixels.
 * @param pitch The width of the frame buffer, in bytes.
 */
static void retro_core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
    if (data && g_retro.counter_connections > 0) {
        void *converted_image = convert_img_to_rgb(data, width, height, pitch, g_retro.video_fmt);
        int image_size;
        unsigned char *png_img = stbi_write_png_to_mem((const unsigned char *) converted_image, 3 * width, width, height, 3, &image_size);
        for (int i=0;i<MAX_CONN;i++){
            if (g_retro.connections[i] != NULL) {
                send_data(CMD_SEND_VIDEO, png_img, image_size, g_retro.connections[i]);
            }
        }
    }
}

/**
 * Polls the inputs. It is used for the hardware the update the actual input states.
 */
static void retro_core_input_poll(void) {
}

/**
 * Queries for input for player 'port'.
 *
 * @param port Which player 'port' to query.
 * @param device Which device type to query for.
 * @param index The input index to retrieve. (?)
 * @param id The ID of which value to query, like \c RETRO_DEVICE_ID_JOYPAD_B.
 * @returns Depends on the provided arguments, but will return 0 if their values are unsupported 
 * by the frontend or the backing physical device. Also, in general it will return 0 (RELEASED) or 1 (PRESSED).
 */
static int16_t retro_core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    return 0;
}

static void retro_core_audio_sample(int16_t left, int16_t right) {
    int16_t buf[2] = {left, right};
	// send_audio(buf, 2);
}

static size_t retro_core_audio_sample_batch(const int16_t *data, size_t frames) {
    unsigned buff_size = sizeof(int16_t) * NUM_AUDIO_CHANNELS * frames;
    // send_audio(buf, buff_size);
    return frames;
}

int load_core(const char *sofile) {
    // Local functions to set the callbacks
    void (*set_environment)(retro_environment_t) = NULL;
    void (*set_video_refresh)(retro_video_refresh_t) = NULL;
    void (*set_input_poll)(retro_input_poll_t) = NULL;
    void (*set_input_state)(retro_input_state_t) = NULL;
    void (*set_audio_sample)(retro_audio_sample_t) = NULL;
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
    
    g_retro.handle = dlopen(sofile, RTLD_LAZY);
    if (!g_retro.handle){
        log_message(LOG_LEVEL_ERROR, "Error loading library: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    // Link to functions in core file
    load_sym(g_retro.retro_init, retro_init, g_retro.handle);
    load_sym(g_retro.retro_deinit, retro_deinit, g_retro.handle);
    load_sym(g_retro.retro_api_version, retro_api_version, g_retro.handle);
    load_sym(g_retro.retro_get_system_info, retro_get_system_info, g_retro.handle);
    load_sym(g_retro.retro_get_system_av_info, retro_get_system_av_info, g_retro.handle);
    load_sym(g_retro.retro_set_controller_port_device, retro_set_controller_port_device, g_retro.handle);
    load_sym(g_retro.retro_load_game, retro_load_game, g_retro.handle);
    load_sym(g_retro.retro_load_game_special, retro_load_game_special, g_retro.handle);
    load_sym(g_retro.retro_unload_game, retro_unload_game, g_retro.handle);
    load_sym(g_retro.retro_serialize_size, retro_serialize_size, g_retro.handle);
    load_sym(g_retro.retro_serialize, retro_serialize, g_retro.handle);
    load_sym(g_retro.retro_unserialize, retro_unserialize, g_retro.handle);
    load_sym(g_retro.retro_reset, retro_reset, g_retro.handle);
    load_sym(g_retro.retro_run, retro_run, g_retro.handle);
    load_sym(g_retro.retro_get_region, retro_get_region, g_retro.handle);
    load_sym(g_retro.retro_get_memory_data, retro_get_memory_data, g_retro.handle);
    load_sym(g_retro.retro_get_memory_size, retro_get_memory_size, g_retro.handle);
    load_sym(g_retro.retro_cheat_reset, retro_cheat_reset, g_retro.handle);
    load_sym(g_retro.retro_cheat_set, retro_cheat_set, g_retro.handle);
    
    load_sym(set_environment, retro_set_environment, g_retro.handle);
    load_sym(set_video_refresh, retro_set_video_refresh, g_retro.handle);
    load_sym(set_input_poll, retro_set_input_poll, g_retro.handle);
    load_sym(set_input_state, retro_set_input_state, g_retro.handle);
    load_sym(set_audio_sample, retro_set_audio_sample, g_retro.handle);
    load_sym(set_audio_sample_batch, retro_set_audio_sample_batch, g_retro.handle);
    
    // Set callbacks
    set_environment(retro_core_environment);
    set_video_refresh(retro_core_video_refresh);
    set_input_poll(retro_core_input_poll);
    set_input_state(retro_core_input_state);
    set_audio_sample(retro_core_audio_sample);
    set_audio_sample_batch(retro_core_audio_sample_batch);

    g_retro.retro_init();
    g_retro.initialized = true;

    return EXIT_SUCCESS;
}

int load_game_from_file(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: Filename is NULL.\n");
        return EXIT_FAILURE;
    }

    struct retro_system_info system = {0};
    struct retro_game_info info = {filename, NULL, 0, NULL};
    FILE *file = fopen(filename, "rb");
    void *game_data = NULL;

    if (!file) {
        log_message(LOG_LEVEL_ERROR, "Error: Failed to open file '%s': %s\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    info.size = ftell(file);
    if (info.size == -1L) {
        log_message(LOG_LEVEL_ERROR, "Error: Failed to determine file size for '%s': %s\n", filename, strerror(errno));
        cleanup(file, NULL);
        return EXIT_FAILURE;
    }
    rewind(file);

    // Get system information
    g_retro.retro_get_system_info(&system);

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
    if (!g_retro.retro_load_game(&info)) {
        log_message(LOG_LEVEL_ERROR, "Failed to load game '%s' into the emulator.\n", filename);
        cleanup(file, game_data);
        return EXIT_FAILURE;
    }
    
    cleanup(file, game_data);

    struct retro_system_av_info av = {0};
    g_retro.retro_get_system_av_info(&av);
    log_message(LOG_LEVEL_DEBUG, "Aspect Ratio: %f", av.geometry.aspect_ratio);
    log_message(LOG_LEVEL_DEBUG, "Height: %d| Width: %d", av.geometry.base_height, av.geometry.base_width);
    log_message(LOG_LEVEL_DEBUG, "Game loaded");

    return EXIT_SUCCESS;
}
