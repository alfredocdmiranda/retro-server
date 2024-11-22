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

void *convert_img_to_rgb(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt) {
	pixel_t *buffer = (pixel_t*)malloc(width * height * 3);
	uint8_t *inbytes = (uint8_t*)data;
	if (fmt == RETRO_PIXEL_FORMAT_XRGB8888) {
		for (unsigned row = 0; row < height; row++) {
			uint32_t *inbuf = (uint32_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = inbuf[col] >> 16;
				buffer[row * width + col].g = inbuf[col] >>  8;
				buffer[row * width + col].b = inbuf[col];
			}
		}
	} else if (fmt == RETRO_PIXEL_FORMAT_RGB565) {
		for (unsigned row = 0; row < height; row++) {
			uint16_t *inbuf = (uint16_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = ((inbuf[col] >> 11) & 0x1F) << 3;
				buffer[row * width + col].g = ((inbuf[col] >>  5) & 0x3F) << 2;
				buffer[row * width + col].b = ((inbuf[col] & 0x1F) << 3);
			}
		}
	} else {
		for (unsigned row = 0; row < height; row++) {
			uint16_t *inbuf = (uint16_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = ((inbuf[col] >> 10) & 0x1F) << 3;
				buffer[row * width + col].g = ((inbuf[col] >>  5) & 0x1F) << 3;
				buffer[row * width + col].b = ((inbuf[col] & 0x1F) << 3);
			}
		}
	}
	return buffer;
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