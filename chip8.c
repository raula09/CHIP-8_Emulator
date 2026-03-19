#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// each character is 5 bytes, sprites are drawn from these
static const uint8_t font[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// zero everything out and set the starting state
void init_chip8(Chip8 *chip8) {
    memset(chip8, 0, sizeof(Chip8));
    chip8->pc = START_ADDRESS;  // programs always start at 0x200
    memcpy(&chip8->memory[FONT_ADDR], font, FONTSET_SIZE);  // load font into memory at 0x050
}

// read the rom file and dump it into memory starting at 0x200
void load_rom(Chip8 *chip8, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "error: could not open ROM '%s'\n", filename);
        return;
    }

    size_t bytes_read = fread(&chip8->memory[START_ADDRESS], 1, MEMORY_SIZE - START_ADDRESS, fp);
    fclose(fp);

    if (bytes_read == 0)
        fprintf(stderr, "error: ROM '%s' is empty or unreadable\n", filename);
    else
        fprintf(stderr, "ROM size: '%s' (%zu bytes)\n", filename, bytes_read);
}

// one cpu cycle — fetch opcode, decode it, run it
void emulate_cycle(Chip8 *chip8) {
    // fetch: grab 2 bytes and merge them into one 16-bit opcode
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    // crack the opcode into its parts upfront so we dont repeat this everywhere
    uint8_t  X   = (opcode & 0x0F00) >> 8;   // second nibble, usually a register index
    uint8_t  Y   = (opcode & 0x00F0) >> 4;   // third nibble, usually a register index
    uint8_t  n   = (opcode & 0x000F);         // fourth nibble
    uint8_t  kk  = (opcode & 0x00FF);         // lower byte
    uint16_t nnn = (opcode & 0x0FFF);         // lower 12 bits, usually an address

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:
                    // clear the screen
                    memset(chip8->gfx, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
                    chip8->draw_flag = 1;
                    break;
                case 0x00EE:
                    // return from subroutine, pop the stack
                    chip8->pc = chip8->stack[--chip8->sp];
                    break;
                default:
                    fprintf(stderr, "unknown opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        case 0x1000:
            // jump to address nnn
            chip8->pc = nnn;
            break;

        case 0x2000:
            // call subroutine at nnn, push current pc onto stack first
            chip8->stack[chip8->sp++] = chip8->pc;
            chip8->pc = nnn;
            break;

        case 0x3000:
            // skip next instruction if Vx == kk
            if (chip8->V[X] == kk) chip8->pc += 2;
            break;

        case 0x4000:
            // skip next instruction if Vx != kk
            if (chip8->V[X] != kk) chip8->pc += 2;
            break;

        case 0x5000:
            // skip next instruction if Vx == Vy
            if (chip8->V[X] == chip8->V[Y]) chip8->pc += 2;
            break;

        case 0x6000:
            // set Vx = kk
            chip8->V[X] = kk;
            break;

        case 0x7000:
            // add kk to Vx, no carry flag
            chip8->V[X] += kk;
            break;

        // all the register arithmetic lives here
        case 0x8000:
            switch (n) {
                case 0x0:
                    chip8->V[X] = chip8->V[Y];
                    break;
                case 0x1:
                    chip8->V[X] |= chip8->V[Y];
                    break;
                case 0x2:
                    chip8->V[X] &= chip8->V[Y];
                    break;
                case 0x3:
                    chip8->V[X] ^= chip8->V[Y];
                    break;
                case 0x4: {
                    // add with carry — VF = 1 if result overflows 8 bits
                    uint16_t sum = chip8->V[X] + chip8->V[Y];
                    chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
                    chip8->V[X]   = (uint8_t)sum;
                    break;
                }
                case 0x5:
                    // subtract, VF = 1 if no borrow
                    chip8->V[0xF] = (chip8->V[X] > chip8->V[Y]) ? 1 : 0;
                    chip8->V[X]  -= chip8->V[Y];
                    break;
                case 0x6:
                    // shift right, save the bit that gets dropped into VF
                    chip8->V[0xF] = chip8->V[X] & 0x1;
                    chip8->V[X] >>= 1;
                    break;
                case 0x7:
                    // Vy - Vx, VF = 1 if no borrow
                    chip8->V[0xF] = (chip8->V[Y] > chip8->V[X]) ? 1 : 0;
                    chip8->V[X]   = chip8->V[Y] - chip8->V[X];
                    break;
                case 0xE:
                    // shift left, save the MSB into VF
                    chip8->V[0xF] = (chip8->V[X] >> 7) & 0x1;
                    chip8->V[X] <<= 1;
                    break;
                default:
                    fprintf(stderr, "unknown 0x8000 opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        case 0x9000:
            // skip if Vx != Vy
            if (chip8->V[X] != chip8->V[Y]) chip8->pc += 2;
            break;

        case 0xA000:
            // set index register I to nnn
            chip8->I = nnn;
            break;

        case 0xB000:
            // jump to nnn + V0
            chip8->pc = nnn + chip8->V[0];
            break;

        case 0xC000:
            // random byte AND'd with kk, stored in Vx
            chip8->V[X] = (rand() % 256) & kk;
            break;

        case 0xD000: {
            // draw sprite at (Vx, Vy), n bytes tall, from memory at I
            // pixels are XOR'd onto the screen, VF = 1 if any pixel gets erased (collision)
            uint8_t x_loc = chip8->V[X];
            uint8_t y_loc = chip8->V[Y];
            chip8->V[0xF] = 0;

            for (int row = 0; row < n; row++) {
                uint8_t sprite = chip8->memory[chip8->I + row];
                for (int col = 0; col < 8; col++) {
                    if ((sprite & (0x80 >> col)) == 0) continue;  // skip unset bits

                    // wrap around screen edges
                    int sx    = (x_loc + col) % SCREEN_WIDTH;
                    int sy    = (y_loc + row) % SCREEN_HEIGHT;
                    int index = sx + (sy * SCREEN_WIDTH);

                    if (chip8->gfx[index]) chip8->V[0xF] = 1;  // collision
                    chip8->gfx[index] ^= 1;
                }
            }
            chip8->draw_flag = 1;
            break;
        }

        // key input opcodes
        case 0xE000:
            switch (kk) {
                case 0x9E:
                    // skip if key in Vx is pressed
                    if (chip8->keypad[chip8->V[X]]) chip8->pc += 2;
                    break;
                case 0xA1:
                    // skip if key in Vx is NOT pressed
                    if (!chip8->keypad[chip8->V[X]]) chip8->pc += 2;
                    break;
                default:
                    fprintf(stderr, "unknown 0xE000 opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        // misc opcodes — timers, memory, input wait, BCD
        case 0xF000:
            switch (kk) {
                case 0x07:
                    // read delay timer into Vx
                    chip8->V[X] = chip8->delay_timer;
                    break;
                case 0x0A: {
                    // block until a key is pressed, store it in Vx
                    // done by rewinding pc if no key is down
                    int key_pressed = 0;
                    for (int i = 0; i < NUM_KEYS; i++) {
                        if (chip8->keypad[i]) {
                            chip8->V[X] = i;
                            key_pressed = 1;
                            break;
                        }
                    }
                    if (!key_pressed) chip8->pc -= 2;
                    break;
                }
                case 0x15:
                    chip8->delay_timer = chip8->V[X];
                    break;
                case 0x18:
                    chip8->sound_timer = chip8->V[X];
                    break;
                case 0x1E:
                    chip8->I += chip8->V[X];
                    break;
                case 0x29:
                    // point I at the font sprite for the digit in Vx
                    chip8->I = FONT_ADDR + (chip8->V[X] & 0x0F) * 5;
                    break;
                case 0x33: {
                    // store BCD of Vx at I, I+1, I+2
                    // e.g. 156 → memory[I]=1, memory[I+1]=5, memory[I+2]=6
                    uint8_t val = chip8->V[X];
                    chip8->memory[chip8->I]     = val / 100;
                    chip8->memory[chip8->I + 1] = (val / 10) % 10;
                    chip8->memory[chip8->I + 2] = val % 10;
                    break;
                }
                case 0x55:
                    // dump V0 through Vx into memory starting at I
                    for (int i = 0; i <= X; i++)
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    break;
                case 0x65:
                    // load V0 through Vx from memory starting at I
                    for (int i = 0; i <= X; i++)
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    break;
                default:
                    fprintf(stderr, "unknown 0xF000 opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        default:
            fprintf(stderr, "unknown opcode: 0x%04X\n", opcode);
            break;
    }
}