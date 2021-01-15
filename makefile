CC = gcc
CFLAGS = -g

all: directories cboy

# compile with debug symbols
debug: directories cboy-debug

directories:
	@mkdir -p ./bin/
	@mkdir -p ./obj/

cboy: obj/main.o obj/cpu.o obj/instructions.o obj/memory.o
	$(CC) -o bin/$@ $^

cboy-debug: obj/main.o obj/cpu.o obj/instructions.o obj/memory.o
	$(CC) $(CFLAGS) -o bin/cboy $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# clean object files directory
clean:
	@rm -rf ./obj/

# clean object files and binary directories
full-clean: clean
	@rm -rf ./bin/
