CHIP-8 Emulator
written in C

---

LANGUAGE : C  
PLATFORM : Linux / macOS / Windows (with SDL2)

---

INTRO

Started this as a way to get better at C, ended up learning a ton about
how CPUs actually work under the hood. Memory management, fetch-decode-execute
cycles, opcode parsing. learned way faster through building this
than reading about it. Honestly one of the better beginner projects out there
if you want something thats actually useful.

---

WHAT IT IS

A CHIP-8 interpreter written in C. CHIP-8 is an interpreted language from
the mid-1970s, originally designed to make game development easier on early
8-bit microcomputers. Simple by modern standards but a great target for a
first emulator.

I Would Recommend Cowgod's (Thomas P. Greene) CHIP-8 Technical Reference.

What's implemented:

    4KB of RAM
    16 general purpose 8-bit registers (V0 through VF)
    16-level call stack
    64x32 monochrome display
    16-key hex keypad
    Delay and sound timers
    All 35 opcodes

---

REQUIREMENTS

    C compiler (gcc or clang)
    SDL2

---

BUILDING

    mkdir build && cd build
    cmake ..
    make

or manually:

    gcc -o chip8 main.c chip8.c display.c -lSDL2

---

RUNNING

    ./chip8 roms/yourrom.ch8

ROMs can be found at https://github.com/kripod/chip8-roms

---

CONTROLS

Original keypad on the left, keyboard mapping on the right:

    1 2 3 C      →      1 2 3 4
    4 5 6 D      →      Q W E R
    7 8 9 E      →      A S D F
    A 0 B F      →      Z X C V

---

FILES

    chip8.h       struct definitions, constants, function declarations
    chip8.c       core emulator — opcodes, CPU cycle, init, ROM loading
    display.c     SDL2 rendering, input handling, window management
    display.h     display struct and function declarations
    main.c        main loop, timing, glue between chip8 and display
    roms/         put your .ch8 files here

---

NOTES

    programs load at 0x200, font data lives at 0x050
    CPU runs at roughly 600Hz (10 cycles per 60Hz frame)
    timers tick down at 60Hz
    display refreshes at 60Hz
    unknown opcodes get logged to stderr instead of silently crashing
shoutout to Dachi Arevadze and his CHIP-8 emulator (https://github.com/Dachacho/chip8-emulator)
for the recommendation to build this project.