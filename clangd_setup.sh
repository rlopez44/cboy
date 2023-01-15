#!/bin/sh
# Small helper script to generate a compile_commands.json
# file for use by clangd tooling. Uses bear (https://github.com/rizsotto/Bear)
make clean && bear -- make
