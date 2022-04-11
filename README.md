# CBoy
A Nintendo Game Boy emulator written in C.
This is the first emulator I've ever tried to build and
also the first substantial project I've written using C.

# Compiling the Emulator
In order to compile the emulator, you must have [SDL2](https://www.libsdl.org/)
installed on your machine (the library and development files).
For example, to install SDL2 on Debian/Ubuntu, run:

    sudo apt install libsdl2-dev

To compile the emulator, run `make` in the project's root directory.
This will compile the emulator in the `bin/` subdirectory. The
corresponding object files will be stored in the `obj/` subdirectory.
To compile with debug symbols, run `make debug` instead. This debug
emulator will be compiled in the `bin/debug/` subdirectory and the
corresponding object files will be stored in the `obj/debug/`
subdirectory.

# Running the Emulator
To run the emulator, simply execute the following: `bin/cboy <rom_file_path>`.
If you want to run the emulator with debug symbols, then use `bin/debug/cboy`
instead.

# Clean Up
To clean up object files used in prior compilations, run `make clean`.
To clean up both object files and the emulator from prior compilations,
run `make full-clean`. These commands are useful if you want to compile
completely from scratch at any point.

# Notes
Currently, the emulator can't be run on Windows because the debug mode
memory dumping functionality writes to the `/tmp/` directory.

# References
* [*DMG-01: How to Emulate a Game Boy*](https://rylev.github.io/DMG-01/public/book/)
* [*Emulation of Nintendo Game Boy (DMG-01)*](https://raw.githubusercontent.com/Baekalfen/PyBoy/master/PyBoy.pdf)
* [GBDev documentation](https://github.com/gbdev/awesome-gbdev)
    * [Instruction set reference](https://gbdev.io/gb-opcodes/optables/)
    * [CPU opcode reference](https://rgbds.gbdev.io/docs/v0.4.2/gbz80.7)
    * [Pan Docs (Game Boy technical reference)](https://gbdev.io/pandocs/)
* [The `jitboy` Game Boy emulator](https://github.com/sysprog21/jitboy)
* [The `GiiBiiAdvance` GB, GBC, and GBA emulator](https://github.com/AntonioND/giibiiadvance)
* [Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf)
