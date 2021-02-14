CC = gcc
CFLAGS = -I./include/
DEBUG_CFLAG = -g

SRC = $(wildcard src/*.c) $(wildcard src/instructions/*.c)
OBJS = $(patsubst src/%.c, obj/%.o, $(SRC))

all: directories cboy

# compile with debug symbols
debug: directories cboy-debug

directories:
	@mkdir -p ./bin/
	@mkdir -p ./obj/instructions/

cboy: $(OBJS)
	$(CC) $(CFLAGS) $^ -o bin/$@

cboy-debug: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) $^ -o bin/cboy

obj/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) -c $< -o $@

# clean object files directory
clean:
	@rm -rf ./obj/

# clean object files and binary directories
full-clean: clean
	@rm -rf ./bin/
