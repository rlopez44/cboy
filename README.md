# CBoy
A Nintendo Game Boy emulator written in C.

# Compiling the Emulator
In order to compile the emulator, you must have [SDL2](https://www.libsdl.org/)
installed on your machine (the library and development files).
For example, to install SDL2 on Debian/Ubuntu, run:

    apt install libsdl2-dev

or on Arch-based systems:

    pacman -S sdl2

or on MacOS using Homebrew:

    brew install sdl2

To compile the emulator, run `make`. This will compile the emulator in
the `bin/` subdirectory. To compile a debug build, run `make debug`
instead. This debug build of the emulator will be compiled in the
`bin/debug/` subdirectory.

# Running the Emulator
To start the emulator, run `bin/cboy <rom_file_path>` for the release
build or `bin/debug/cboy <rom_file_path>` for the debug build .

# Clean Up
To clean up object files used in prior compilations, run `make clean`.
To clean up both object files and the emulator from prior compilations,
run `make full-clean`.

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
