CC = gcc
CFLAGS = -I./include/
DEBUG_CFLAG = -g
OBJS = obj/main.o obj/cpu.o obj/memory.o
OBJS += obj/instructions/main.o obj/instructions/load.o obj/instructions/arithmetic.o

all: directories cboy

# compile with debug symbols
debug: directories cboy-debug

directories:
	@mkdir -p ./bin/
	@mkdir -p ./obj/instructions/

cboy: $(OBJS)
	$(CC) $(CFLAGS) -o bin/$@ $^

cboy-debug: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) -o bin/cboy $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# clean object files directory
clean:
	@rm -rf ./obj/

# clean object files and binary directories
full-clean: clean
	@rm -rf ./bin/
