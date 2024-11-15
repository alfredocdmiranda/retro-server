# Define the name of the build folder
BUILD_DIR = build

# Define the name of the destination folder
DEST_DIR = bin

# Define the name of the executable
TARGET = retro_server

# List all source files (C) in the project
SOURCES := main.c

# List all header files (H) in the project
HEADERS := libretro.h

# Compiler options
CC = gcc
CFLAGS = -Wall -O2

# All targets for Makefile
all: $(TARGET)

# Rule to compile C source files into an executable
$(TARGET): $(SOURCES) $(HEADERS)
	@echo "Building $(TARGET)"
	@mkdir -p $(DEST_DIR) $(BUILD_DIR)
	@$(CC) $(CFLAGS) -o $@ $(SOURCES) $(HEADERS)

# Clean up the project by removing the compiled executable and object files
clean:
	rm -f $(DEST_DIR)/$(TARGET) $(BUILD_DIRFabrica)/*.o

# Print help information for users
help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all       - Build the executable"
	@echo "  clean     - Remove compiled executable and object files"
	@echo "  help      - Display this help message"