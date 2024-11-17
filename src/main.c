#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"
#include "emulation.h"

struct {
    char *core_path;
    char *rom_path;
    uint16_t port;
} settings;

void help() {
    printf("\nretro_server <corePath> <romPath> [-p portNumber] [-h]\n");
    printf("  corePath          Libretro Core .so file. It always necessary to use fullpath or aliases like \".\" or \"~\".\n");
    printf("  romPath           Rom file.\n");
    printf("  -p                Select the port number. Default: %d\n", DEFAULT_PORT);
    printf("  -h                Show this message\n");
    exit(EXIT_SUCCESS);
}

void read_arguments(int argc, char *argv[]) {
    int opt;
    bool error = false;
    while ((opt = getopt(argc, argv, ":hp:")) != -1) {
        switch (opt) {
        case 'h':
            help();
            break;
        case 'p':
            settings.port = atoi(optarg);
            break;
        case ':':
            printf("needs a value\n");
            help();
            break;
        case '?':
            printf("unknown option: %c\n", opt);
            help();
            break;
        }
    }

    if (optind < argc) {
        for (int index = optind; index < argc; index++) {
            switch (index - optind + 1) {
            case 1:
                settings.core_path = malloc(sizeof(char) * strlen(argv[index]));
                strcpy(settings.core_path, argv[index]);
                break;
            case 2:
                settings.rom_path = malloc(sizeof(char) * strlen(argv[index]));
                strcpy(settings.rom_path, argv[index]);
                break;
            default:
                break;
            }
        }
    }

    if (settings.core_path == NULL) {
        fprintf(stderr, "Missing Positional Argument: Core Path\n");
        error = true;
    }
    if (settings.rom_path == NULL) {
        fprintf(stderr, "Missing Positional Argument: Rom Path\n");
        error = true;
    }

    if (settings.port == 0) {
        settings.port = DEFAULT_PORT;
    }

    if (error) {
        help();
    }
}

void run_emulation() {
    struct timespec start_frame_execution = {0,0};
    struct timespec end_loop_execution={0,0};
    struct retro_system_av_info av = {0};
    
    g_retro.retro_get_system_av_info(&av);
    double delta_frames = 1/(av.timing.fps); // Time between frames in seconds
    double delta = delta_frames;
    log_message(LOG_LEVEL_DEBUG, "Emulation started");
    while (true) {
        if (delta >= delta_frames) {
            // Ensure that executes the correct amount of FPS.
            clock_gettime(CLOCK_MONOTONIC, &start_frame_execution);
            g_retro.retro_run();
            delta = delta-delta_frames;
        }
        clock_gettime(CLOCK_MONOTONIC, &end_loop_execution);
        delta = ((double)end_loop_execution.tv_sec + 1.0e-9*end_loop_execution.tv_nsec) - 
                ((double)start_frame_execution.tv_sec + 1.0e-9*start_frame_execution.tv_nsec);
    }
}

int main(int argc, char *argv[]) {
    read_arguments(argc, argv);
    load_core(settings.core_path);
    load_game_from_file(settings.rom_path);
    run_emulation();

    return EXIT_SUCCESS;
}