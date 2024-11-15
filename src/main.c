#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "constants.h"



struct {
    char *core_path;
    char *rom_path;
    uint16_t port;
} settings;

void help() {
    printf("%s\n\n", APP_NAME);
    printf("retro_server -c corePath -r romPath [-p portNumber] [-h]\n");
    printf("  -c\t\tSelect the Core .so file.\n");
    printf("  -r\t\tSelect the Rom file.\n");
    printf("  -p\t\tSelect the port number. Default: %d\n", DEFAULT_PORT);
    printf("  -h\t\tShow this message\n");
    exit(0);
}

void read_arguments(int argc, char *argv[]) {
    int opt;
    bool error = false;
    while ((opt = getopt(argc, argv, ":hc:p:r:")) != -1) {
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
        printf("Positional arguments: %d\n", optind);
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
            printf("  Argument %d: %s\n", index - optind + 1, argv[index]);
        }
    } else {
        fprintf(stderr, "Error: Missing positional arguments.\n");
        error = true;
        return;
    }

    if (settings.core_path == NULL) {
        printf("Missing Core Path\n");
        error = true;
    } else if (settings.rom_path == NULL) {
        printf("Missing Rom Path\n");
        error = true;
    } else if (settings.port == 0) {
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