CC = gcc
CFLAGS = -Wall -I./include/
OBJ_DIR = obj
BIN_DIR = bin
DEBUG_DIR = debug
BIN = cboy

# so we can reference all our source files without directories
vpath %.c src/ src/instructions/

# list of all our source files without directories
SRC = $(notdir $(wildcard src/*.c) $(wildcard src/instructions/*.c))

# list of object file names for debug and regular builds
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))
DEBUG_OBJS = $(patsubst %.c, $(OBJ_DIR)/$(DEBUG_DIR)/%.o, $(SRC))

all: CFLAGS += -O3
all: $(BIN_DIR)/$(BIN)

debug: CFLAGS += -g
debug: $(BIN_DIR)/$(DEBUG_DIR)/$(BIN)

# rules for making required directories
$(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/$(DEBUG_DIR) $(OBJ_DIR)/$(DEBUG_DIR):
	mkdir -p $@/

# regular build
$(BIN_DIR)/$(BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# debug build
$(BIN_DIR)/$(DEBUG_DIR)/$(BIN): $(DEBUG_OBJS) | $(BIN_DIR)/$(DEBUG_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# object files
.SECONDEXPANSION:
%.o: $$(*F).c | $$(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# clean object files directory
clean:
	rm -rf $(OBJ_DIR)/

# clean object files and binary directories
full-clean: clean
	rm -rf $(BIN_DIR)/
