#include <time.h>

#include "utils.h"

void cleanup(FILE *file, void *data) {
    if (file) {
         fclose(file);
    }
    if (data) {
        free(data);
    } 
}

void get_timestamp(char *timestamp, size_t len) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

void log_message(int level, const char *format, ...) {
    if (level >= LOG_LEVEL) {
        const char *level_str;
        switch (level) {
            case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
            case LOG_LEVEL_INFO: level_str = "INFO"; break;
            case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
            case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
            default: level_str = "UNKNOWN"; break;
        }

        // Get the current timestamp
        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));

        // Print the log level and the message
        printf("[%s] [%s] ", timestamp, level_str);

        // Handle variable arguments
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        // Print a newline at the end
        printf("\n");
    }
}