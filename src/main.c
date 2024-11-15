#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "constants.h"



struct {
    char *core_path;
    char *rom_path;
    uint16_t port;
} settings;

void help() {
    printf("\nretro_server corePath romPath [-p portNumber] [-h]\n");
    printf("  corePath          Libretro Core .so file. Tip: If the file is in the same directoy you need to use \"./<file>\".\n");
    printf("  romPath           Rom file.\n");
    printf("  -p                Select the port number. Default: %d\n", DEFAULT_PORT);
    printf("  -h                Show this message\n");
    exit(0);
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

int main(int argc, char *argv[]) {
    read_arguments(argc, argv);
    
    return 0;
}