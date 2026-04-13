CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2 -I src
LDFLAGS ?=

SRCS := \
	src/main.c \
	src/sys.c \
	src/string.c \
	src/memory.c \
	src/vfs.c \
	src/scheduler.c \
	src/shell.c

.PHONY: all clean run

all: minios

minios: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f minios

run: minios
	./minios
