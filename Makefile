CC=gcc
CFLAGS=-lncurses
OUT=bin/typewriter
TARGETS=src/*.c

all:
	$(CC) $(TARGETS) $(CFLAGS) -o $(OUT)

gdb:
	$(CC) $(TARGETS) $(CFLAGS) -g -o $(OUT)

test:
	$(MAKE) all
	./$(OUT)
