# Define the name of the build folder
BUILD_DIR = build

# Define the name of the destination folder
DEST_DIR = bin

# Define the name of the executable
TARGET = retro_server

# List all source files (C) in the project
SOURCES := $(wildcard retro-server-emulator/*.c retro-server-emulator/**/*.c)

# Compiler options
CC = gcc
CFLAGS = -Wall -O2
LFLAGS   := -static-libgcc
LIBS     :=
packages := sdl2 libpng

ifneq ($(packages),)
    LIBS    += $(shell pkg-config --libs-only-l $(packages))
    LFLAGS  += $(shell pkg-config --libs-only-L --libs-only-other $(packages))
    CFLAGS  += $(shell pkg-config --cflags $(packages))
endif

# All targets for Makefile
all: $(TARGET)

# Rule to compile C source files into an executable
$(TARGET): $(SOURCES) $(HEADERS)
	@echo "Building $(SOURCES)"
	@mkdir -p $(DEST_DIR) $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(LFLAGS) -o $(DEST_DIR)/$@ $(SOURCES) $(LIBS)

# Clean up the project by removing the compiled executable and object files
clean:
	rm -f $(DEST_DIR)/$(TARGET) $(BUILD_DIR)/*.o

# Print help information for users
help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all       - Build the executable"
	@echo "  clean     - Remove compiled executable and object files"
	@echo "  help      - Display this help message"