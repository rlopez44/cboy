# CBoy
A Nintendo Game Boy emulator written in C.
I've never made an emulator before, and I've also never programmed anything substantial
using C, so I'm kinda jumping off the deep end here. Below you'll find the references
I'm using as I build this out.

# Compiling the Emulator
To compile the emulator, run `make` in the project's root directory. To compile with debug symbols,
run `make debug` instead. The emulator will be compiled in the `bin/` subdirectory.

# Clean Up
The `make` and `make debug` commands create `obj/` and `bin/` subdirectories for storing object files
and the emulator, respectively. To clean up the `obj/` directory, run `make clean`. To clean up the
`obj/` and `bin/` directories, run `make full-clean`.

# References
* [*DMG-01: How to Emulate a Game Boy*](https://rylev.github.io/DMG-01/public/book/)
* [*Emulation of Nintendo Game Boy (DMG-01)*](https://raw.githubusercontent.com/Baekalfen/PyBoy/master/PyBoy.pdf)
* [GBDev documentation](https://github.com/gbdev/awesome-gbdev)
    * [Instruction set reference](https://gbdev.io/gb-opcodes/optables/)
    * [CPU opcode reference](https://rgbds.gbdev.io/docs/v0.4.2/gbz80.7)
    * [Pan Docs (Game Boy technical reference)](https://gbdev.io/pandocs/)
* [The `jitboy` Game Boy emulator](https://github.com/sysprog21/jitboy)
* [The `GiiBiiAdvance` GB, GBC, and GBA emulator](https://github.com/AntonioND/giibiiadvance)
