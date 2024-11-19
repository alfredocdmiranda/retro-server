#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "communication/server.h"
#include "constants.h"
#include "emulation.h"

typedef struct {
    int index;
    int** client_socket;
} thread_args_t;

struct {
    char *core_path;
    char *rom_path;
    uint16_t port;
} settings;
struct sockaddr_in server_addr;
int server_socket;

int* connections[MAX_CONN] = {NULL};
int counter_connections = 0;
pthread_mutex_t conn_counter_mutex;

void *client_handler(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int client_socket = **(args->client_socket);

    char buffer[1024];
    int bytes_read;

    log_message(LOG_LEVEL_INFO, "Player connected to slot %d", args->index);

    pthread_mutex_lock(&conn_counter_mutex);
    counter_connections++;
    pthread_mutex_unlock(&conn_counter_mutex);
    
    // Communicate with the client
    while ((bytes_read = read(client_socket, buffer, 1024)) > 0) {
    }

    if (bytes_read == 0) {
        log_message(LOG_LEVEL_INFO, "Player disconnected from slot %d\n", args->index);
    } else {
        log_message(LOG_LEVEL_ERROR, "Read error from slot %d", args->index);
    }

    pthread_mutex_lock(&conn_counter_mutex);
    counter_connections--;
    pthread_mutex_unlock(&conn_counter_mutex);

    *(args->client_socket) = NULL;
    close(client_socket); // Close the connection
    pthread_exit(NULL); // Exit the thread
}

void *run_emulation(void *arg) {
    struct timespec start_frame_execution = {0,0};
    struct timespec start_loop_execution={0,0};
    struct timespec end_loop_execution={0,0};
    struct retro_system_av_info av = {0};
    
    g_retro.retro_get_system_av_info(&av);
    double delta_frames = 1/(av.timing.fps); // Time between frames in seconds
    double delta = delta_frames;
    double execution_time = 0;
    int fps_counter = 0;

    log_message(LOG_LEVEL_DEBUG, "Emulation started");
    while (true) {
        if (counter_connections == 0) {
            // Pause the emulation if there is no one connected.
            delta = delta_frames;
            log_message(LOG_LEVEL_DEBUG, "No connection");
            continue;
        }

        if (execution_time >= 1) {
            // Show the amount of FPS to check the performance
            clock_gettime(CLOCK_MONOTONIC, &start_loop_execution);
            log_message(LOG_LEVEL_DEBUG, "FPS: %d", fps_counter);
            fps_counter = 0;
            execution_time = 0;
        }
        if (delta >= delta_frames) {
            // Ensure that executes the correct amount of FPS.
            clock_gettime(CLOCK_MONOTONIC, &start_frame_execution);
            g_retro.retro_run();
            delta = delta-delta_frames;
            fps_counter++;
        }
        clock_gettime(CLOCK_MONOTONIC, &end_loop_execution);
        delta = ((double)end_loop_execution.tv_sec + 1.0e-9*end_loop_execution.tv_nsec) - 
                ((double)start_frame_execution.tv_sec + 1.0e-9*start_frame_execution.tv_nsec);
        execution_time = ((double)end_loop_execution.tv_sec + 1.0e-9*end_loop_execution.tv_nsec) - 
                ((double)start_loop_execution.tv_sec + 1.0e-9*start_loop_execution.tv_nsec);
    }
}

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

int main(int argc, char *argv[]) {
    pthread_t emulation_thread_id;

    read_arguments(argc, argv);
    load_core(settings.core_path);
    load_game_from_file(settings.rom_path);

    create_server(&server_socket, &server_addr, settings.port);

    if (pthread_mutex_init(&conn_counter_mutex, NULL) != 0) {
        log_message(LOG_LEVEL_ERROR, "Mutex initialization failed");
        return EXIT_FAILURE;
    }

    if (pthread_create(&emulation_thread_id, NULL, run_emulation, NULL) != 0) {
        log_message(LOG_LEVEL_ERROR, "Emulation Thread creation failed.");
        return EXIT_FAILURE;
    }

    while(true) {
        int i;
        for (i=0;i < MAX_CONN;i++) {
            if (connections[i] == NULL) {
                break;
            }
        }
        if (i == MAX_CONN) {
            continue;
        }
        
        connections[i] = malloc(sizeof(int));
        int conn_status = wait_connection(&server_addr, &server_socket, connections[i]);
        if (conn_status != 0) {
            log_message(LOG_LEVEL_ERROR, "It could not stabilish a connection.");
            continue;
        }

        pthread_t thread_id;
        thread_args_t args = {i, &connections[i]};
        if (pthread_create(&thread_id, NULL, client_handler, &args) != 0) {
            log_message(LOG_LEVEL_ERROR, "Thread creation failed.");
            close(*connections[i]);
            free(connections[i]);
            connections[i] = NULL;
            continue;
        }
        pthread_detach(thread_id);
    }

    pthread_mutex_destroy(&conn_counter_mutex);

    return EXIT_SUCCESS;
}