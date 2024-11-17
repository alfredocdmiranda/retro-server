#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR 4

#define LOG_LEVEL LOG_LEVEL_DEBUG

// Function to clean up resources in case of an error
void cleanup(FILE *file, void *data);
void get_timestamp(char *timestamp, size_t len);
void log_message(int level, const char *format, ...);

#endif