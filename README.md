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

There are three builds of the emulator available: a release build,
a profiling build (for use with `gprof`), and a debug build; these
are created with `make`, `make profile`, and `make debug`,
respectively. The corresponding executables are located at `bin/cboy`,
`bin/profile/cboy`, and `bin/debug/cboy`.

# Running the Emulator
The emulator accepts a game ROM file and, optionally,
a DMG boot ROM file to play before starting the game ROM. For example,
the release mode of the emulator is invoked as follows:
`bin/cboy [-b bootrom] <romfile>`.

# Clean Up
To clean up object files used in prior compilations, run `make clean`.
To clean up both object files and the emulator from prior compilations,
run `make full-clean`.

# Notes
Currently, the emulator can't be run on Windows because the debug mode
memory dumping functionality writes to the `/tmp/` directory and because
the POSIX `getopt` function is used for command line options processing.

# Supported MBCs
CBoy currently supports the following Memory Bank Controllers:
* No MBC
* MBC1 (except for MBC1M)
* MBC3 (no RTC)
* MBC5 (no rumble)

# Test ROM Coverage
The emulator currently passes the following test ROMS.
* Blargg
    * cpu_instrs
    * instr_timing
* [Scribbltests](https://github.com/Hacktix/scribbltests)
    * Fairylake
    * LYCSCX
    * LYCSCY
    * PaletteLY
    * SCXLY
    * WinPos
* [Mooneye Test Suite](https://github.com/Gekkio/mooneye-test-suite)
    * `emulator-only/mbc1/*` except for `multicart_rom_8Mb.s`
    * `emulator-only/mbc5/*`
    * `acceptance/bits/{mem_oam,reg_f}.s`
    * `acceptance/instr/*`
    * `acceptance/oam_dma/{basic,reg_read}.s`
* [dmg-acid2](https://github.com/mattcurrie/dmg-acid2)

# References
* [*DMG-01: How to Emulate a Game Boy*](https://rylev.github.io/DMG-01/public/book/)
* [*Emulation of Nintendo Game Boy (DMG-01)*](https://raw.githubusercontent.com/Baekalfen/PyBoy/master/PyBoy.pdf)
  (note: this is a download link)
* [GBDev documentation](https://github.com/gbdev/awesome-gbdev)
    * [Instruction set reference](https://gbdev.io/gb-opcodes/optables/)
    * [CPU opcode reference](https://rgbds.gbdev.io/docs/v0.4.2/gbz80.7)
    * [Pan Docs (Game Boy technical reference)](https://gbdev.io/pandocs/)
* [The `jitboy` Game Boy emulator](https://github.com/sysprog21/jitboy)
* [The `GiiBiiAdvance` GB, GBC, and GBA emulator](https://github.com/AntonioND/giibiiadvance)
* [Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf)
