CC = gcc
CFLAGS = -I./include/
DEBUG_CFLAG = -g

SRC = $(wildcard src/*.c) $(wildcard src/instructions/*.c)
OBJS = $(patsubst src/%.c, obj/%.o, $(SRC))

BIN = cboy

all: directories build-emulator

# compile with debug symbols
debug: directories build-emulator-debug

directories:
	@mkdir -p ./bin/
	@mkdir -p ./obj/instructions/

# strip debug symbols from the executable after compiling
build-emulator: $(OBJS)
	$(CC) $(CFLAGS) $^ -o bin/$(BIN)
	strip $(DEBUG_CFLAG) bin/$(BIN) -o bin/$(BIN)

build-emulator-debug: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) $^ -o bin/$(BIN)

# add debug symbols to the object files
obj/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) -c $< -o $@

# clean object files directory
clean:
	@rm -rf ./obj/

# clean object files and binary directories
full-clean: clean
	@rm -rf ./bin/
