CC = gcc
CFLAGS = -I./include/
DEBUG_CFLAG = -g

OBJ_DIR = obj
BIN_DIR = bin

BIN = cboy

# so we can reference all our source files without directories
vpath %.c src/ src/instructions/

# list of all our source files without directories
SRC = $(notdir $(wildcard src/*.c) $(wildcard src/instructions/*.c))
# list of object file names without directories
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))

all: build-emulator

# compile with debug symbols
debug: build-emulator-debug

# rules for making required directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)/

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/

# strip debug symbols from the executable after compiling
build-emulator: $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$(BIN)
	strip $(DEBUG_CFLAG) $(BIN_DIR)/$(BIN) -o $(BIN_DIR)/$(BIN)

build-emulator-debug: $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) $^ -o $(BIN_DIR)/$(BIN)

# add debug symbols to the object files
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAG) -c $< -o $@

# clean object files directory
clean:
	rm -rf $(OBJ_DIR)/

# clean object files and binary directories
full-clean: clean
	rm -rf $(BIN_DIR)/
