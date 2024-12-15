TARGET = bin/download
LIBS = -lm
CC = gcc
CFLAGS = -Wall

# Default arguments
ARG1 = ftp://ftp.up.pt/pub/kodi/timestamp.txt
ARG2 = ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt
ARG3 = ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4
ARG4 = ftp://ftp.gnu.org/gnu/gdb/gdb-5.2.1.tar.gz
ARG5 = ftp://anonymous:anonymous@ftp.up.pt/pub/debian/doc/social-contract.txt

.PHONY: default all clean

default: $(TARGET)
all: default

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Source and object files
SOURCES = $(wildcard $(SRC_DIR)/*.c) main.c
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SOURCES))
OBJECTS := $(patsubst main.c, $(BIN_DIR)/main.o, $(OBJECTS))

# Compile source files to object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/main.o: main.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into the final executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET)

# Run the program with dynamic arguments
run: $(TARGET)
	./$(TARGET) $(URL)

# Clean up the build files
clean:
	-rm -f $(BIN_DIR)/*.o $(TARGET)
