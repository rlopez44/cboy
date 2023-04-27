CC = gcc
CFLAGS = -Wall -Wextra -pedantic -I./include/ -std=c17
CFLAGS += `sdl2-config --cflags`
LDLIBS = `sdl2-config --libs`
OBJ_DIR = obj
BIN_DIR = bin
PROFILE_DIR = profile
DEBUG_DIR = debug
INSTALL_DIR = /usr/local/bin
BIN = cboy

# so we can reference all our source files without directories
vpath %.c src/ src/instructions/ src/mbcs/

# list of all our source files without directories
SRC = $(notdir $(wildcard src/*.c) $(wildcard src/instructions/*.c) $(wildcard src/mbcs/*.c))

# list of object file names for debug, profiling, and release builds
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))
PROFILE_OBJS = $(patsubst %.c, $(OBJ_DIR)/$(PROFILE_DIR)/%.o, $(SRC))
DEBUG_OBJS = $(patsubst %.c, $(OBJ_DIR)/$(DEBUG_DIR)/%.o, $(SRC))

# file dependencies for debug, profiling, and release builds
# these will be created by gcc when compiling object files
DEPENDS = $(patsubst %.o, %.d, $(OBJS))
PROFILE_DEPENDS = $(patsubst %.o, %.d, $(PROFILE_OBJS))
DEBUG_DEPENDS = $(patsubst %.o, %.d, $(DEBUG_OBJS))

.PHONY: all profile debug install clean full-clean

all: CFLAGS += -O3 -flto=auto
all: $(BIN_DIR)/$(BIN)

profile: CFLAGS += -O3 -flto=auto -pg
profile: $(BIN_DIR)/$(PROFILE_DIR)/$(BIN)

debug: CFLAGS += -g -DDEBUG
debug: $(BIN_DIR)/$(DEBUG_DIR)/$(BIN)

# rules for making required directories
$(BIN_DIR) $(OBJ_DIR)\
$(BIN_DIR)/$(PROFILE_DIR) $(OBJ_DIR)/$(PROFILE_DIR)\
$(BIN_DIR)/$(DEBUG_DIR) $(OBJ_DIR)/$(DEBUG_DIR):
	mkdir -p $@/

# regular build
$(BIN_DIR)/$(BIN): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

# profiling build
$(BIN_DIR)/$(PROFILE_DIR)/$(BIN): $(PROFILE_OBJS) | $(BIN_DIR)/$(PROFILE_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

# debug build
$(BIN_DIR)/$(DEBUG_DIR)/$(BIN): $(DEBUG_OBJS) | $(BIN_DIR)/$(DEBUG_DIR)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

-include $(DEPENDS) $(PROFILE_DEPENDS) $(DEBUG_DEPENDS)

# object files (plus dependency files from -MMD -MP)
.SECONDEXPANSION:
%.o: $$(*F).c makefile | $$(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# clean object files directory
clean:
	rm -rf $(OBJ_DIR)/

# clean object files and binary directories
full-clean: clean
	rm -rf $(BIN_DIR)/

install: all
	cp $(BIN_DIR)/$(BIN) $(INSTALL_DIR)
