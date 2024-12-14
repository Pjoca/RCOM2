TARGET = download
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall
ARG1 = ftp://ftp.up.pt/pub/kodi/timestamp.txt
ARG2 = ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt
ARG3 = ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4
ARG4 = ftp://ftp.gnu.org/gnu/gdb/gdb-5.2.1.tar.gz

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

run: ${TARGET}
	./${TARGET} ${ARGS} ${ARG1}

clean:
	-rm -f *.o
	-rm -f $(TARGET)