# Architecture (CHIP-8_Emulator)

This repo is a small CHIP-8 interpreter written in C, with SDL2 used for windowing, rendering, and keyboard input.

## Build/Run

- CMake entry: `CMakeLists.txt`
- Manual build (also in `README.md`):
  - `gcc -o chip8 main.c chip8.c display.c -lSDL2`
- Run:
  - `./chip8 roms/Tetris.ch8`

## High-Level Data Flow

1. Initialize the CHIP-8 VM state (`init_chip8`): zero state, set `pc = 0x200`, load font sprites into memory.
2. Load ROM bytes into memory starting at `0x200` (`load_rom`).
3. Initialize SDL display (`display_init`): create window, renderer, and a 64x32 RGBA texture.
4. Main loop (`main.c`):
   - Poll SDL events and update `chip8.keypad[]`.
   - Execute a fixed number of CHIP-8 instructions per frame (`emulate_cycle` in a loop).
   - Decrement `delay_timer`/`sound_timer` once per frame.
   - If `chip8.draw_flag` is set, redraw from `chip8.gfx[]`.
   - Sleep ~16ms (`SDL_Delay(16)`) to target ~60Hz.

## Module Ownership

- `main.c`
  - Program entry, SDL event pump, fixed-rate CPU stepping, timer ticking, conditional rendering.
- `chip8.h`
  - CHIP-8 constants (memory/register sizes, screen dimensions, memory layout).
  - `Chip8` struct: all VM state (memory, registers, stack, timers, framebuffer, keypad).
  - Public functions: `init_chip8`, `load_rom`, `emulate_cycle`.
- `chip8.c`
  - Built-in font set (hex digits 0..F, 5 bytes per digit).
  - ROM loading into `memory[]`.
  - Instruction fetch/decode/execute in `emulate_cycle()`.
- `display.h` / `display.c`
  - SDL2 wrapper (`Display`) and functions:
    - init/destroy SDL resources
    - convert `chip8.gfx[]` -> RGBA pixels and present
    - map host keyboard to CHIP-8 keypad and update `chip8.keypad[]`

## Memory Map (Conventional CHIP-8)

- `0x000..0x1FF`: historically reserved/interpreter space
- `0x050..0x09F`: font sprites in this implementation (`FONT_ADDR = 0x50`, 80 bytes)
- `0x200..`: program start (`START_ADDRESS = 0x200`)

ROM bytes are loaded into `memory[0x200...]` until either EOF or RAM is full.

## Display Model

- Logical resolution: 64x32 monochrome (`SCREEN_WIDTH = 64`, `SCREEN_HEIGHT = 32`)
- Storage: `chip8.gfx[64*32]` with values 0/1
- Draw opcode (`Dxyn`) XORs sprite pixels into `gfx[]` and sets `VF` on collision.
- Rendering (`display_draw`) converts `gfx[]` into a 64x32 RGBA buffer:
  - on -> `0xFFFFFFFF`
  - off -> `0x000000FF`

SDL is configured with `SDL_RenderSetLogicalSize(renderer, 64, 32)` so the window scales cleanly.

## Input Model

CHIP-8 has a 16-key hexadecimal keypad. This project maps it to a typical PC keyboard using SDL scancodes:

- CHIP-8: `1 2 3 C` -> Keyboard: `1 2 3 4`
- CHIP-8: `4 5 6 D` -> Keyboard: `Q W E R`
- CHIP-8: `7 8 9 E` -> Keyboard: `A S D F`
- CHIP-8: `A 0 B F` -> Keyboard: `Z X C V`

On `SDL_KEYDOWN`/`SDL_KEYUP`, `display_handle_input()` sets `chip8.keypad[i]` to 1/0 for the mapped key.

## Timing Model (Current Implementation)

The emulator uses a simple "frame loop" timing model:

- Each loop iteration:
  - executes 10 CHIP-8 instructions (`for (i=0; i<10; i++) emulate_cycle(&chip8);`)
  - decrements timers once (`if (delay_timer > 0) delay_timer--;` etc.)
  - sleeps ~16ms (`SDL_Delay(16)`)

If the loop stays near 16ms, this approximates:

- ~60 frames/sec
- ~600 instructions/sec
- timers at ~60Hz

This is not delta-time based; on slow/fast machines the effective rate may drift.

## Instruction Decode/Execute

`emulate_cycle()`:

- Fetch: `opcode = memory[pc] << 8 | memory[pc+1]`
- Default advance: `pc += 2`
- Decode helper fields:
  - `X = (opcode & 0x0F00) >> 8`
  - `Y = (opcode & 0x00F0) >> 4`
  - `n  = (opcode & 0x000F)`
  - `kk = (opcode & 0x00FF)`
  - `nnn = (opcode & 0x0FFF)`
- Execute: `switch(opcode & 0xF000)` with sub-switches for `0x0`, `0x8`, `0xE`, `0xF`.

### Opcode Coverage (Implemented)

- `00E0` CLS
- `00EE` RET
- `1nnn` JP addr
- `2nnn` CALL addr
- `3xkk` SE Vx, byte
- `4xkk` SNE Vx, byte
- `5xy?` SE Vx, Vy (classic form is `5xy0`)
- `6xkk` LD Vx, byte
- `7xkk` ADD Vx, byte
- `8xy0` LD Vx, Vy
- `8xy1` OR Vx, Vy
- `8xy2` AND Vx, Vy
- `8xy3` XOR Vx, Vy
- `8xy4` ADD Vx, Vy (with carry in `VF`)
- `8xy5` SUB Vx, Vy (no-borrow flag in `VF`)
- `8xy6` SHR Vx (LSB -> `VF`)
- `8xy7` SUBN Vx, Vy
- `8xyE` SHL Vx (MSB -> `VF`)
- `9xy?` SNE Vx, Vy (classic form is `9xy0`)
- `Annn` LD I, addr
- `Bnnn` JP V0, addr
- `Cxkk` RND Vx, byte
- `Dxyn` DRW Vx, Vy, nibble
- `Ex9E` SKP Vx
- `ExA1` SKNP Vx
- `Fx07` LD Vx, DT
- `Fx0A` LD Vx, K (blocking: repeats instruction until a key is pressed)
- `Fx15` LD DT, Vx
- `Fx18` LD ST, Vx
- `Fx1E` ADD I, Vx
- `Fx29` LD F, Vx
- `Fx33` LD B, Vx
- `Fx55` LD [I], V0..Vx
- `Fx65` LD V0..Vx, [I]

Unknown opcodes are logged to stderr in the relevant switch arm.

## Compatibility Notes / Quirks

These are common places CHIP-8 emulators differ; this repo currently behaves as follows:

- `5xy?` and `9xy?`:
  - The classic specs define only `5xy0` and `9xy0`, but this implementation does not check the low nibble.
- Shifts (`8xy6`, `8xyE`):
  - Shifts operate on `Vx` directly (some interpreters use `Vy` as the source).
- `Fx0A` (wait for key):
  - Implemented by rewinding `pc` by 2 if no key is pressed, so the opcode re-executes next cycle.
- Sound:
  - `sound_timer` triggers a terminal bell (`printf("\a")`) when it hits zero; SDL audio is initialized but not used.
- API mismatch:
  - `chip8.h` declares `execute_opcode()` but the code executes inside `emulate_cycle()`; there is no `execute_opcode()` implementation.

