#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

void init_chip8(Chip8 *chip8) {
    memset(chip8, 0, sizeof(Chip8));
    chip8->pc = START_ADDRESS;
    memcpy(&chip8->memory[FONT_ADDR], font, FONTSET_SIZE);
}

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
        fprintf(stderr, "loaded ROM '%s' (%zu bytes)\n", filename, bytes_read);
}

void emulate_cycle(Chip8 *chip8) {
    uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    uint8_t  X   = (opcode & 0x0F00) >> 8;
    uint8_t  Y   = (opcode & 0x00F0) >> 4;
    uint8_t  n   = (opcode & 0x000F);
    uint8_t  kk  = (opcode & 0x00FF);
    uint16_t nnn = (opcode & 0x0FFF);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:
                    memset(chip8->gfx, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
                    chip8->draw_flag = 1;
                    break;
                case 0x00EE:
                    chip8->pc = chip8->stack[--chip8->sp];
                    break;
            }
            break;

        case 0x1000:
            chip8->pc = nnn;
            break;

        case 0x2000:
            chip8->stack[chip8->sp++] = chip8->pc;
            chip8->pc = nnn;
            break;

        case 0x3000:
            if (chip8->V[X] == kk) chip8->pc += 2;
            break;

        case 0x4000:
            if (chip8->V[X] != kk) chip8->pc += 2;
            break;

        case 0x5000:
            if (chip8->V[X] == chip8->V[Y]) chip8->pc += 2;
            break;

        case 0x6000:
            chip8->V[X] = kk;
            break;

        case 0x7000:
            chip8->V[X] += kk;
            break;

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
                    uint16_t sum = chip8->V[X] + chip8->V[Y];
                    chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
                    chip8->V[X]   = (uint8_t)sum;
                    break;
                }
                case 0x5:
                    chip8->V[0xF] = (chip8->V[X] > chip8->V[Y]) ? 1 : 0;
                    chip8->V[X]  -= chip8->V[Y];
                    break;
                case 0x6:
                    chip8->V[0xF] = chip8->V[X] & 0x1;
                    chip8->V[X] >>= 1;
                    break;
                case 0x7:
                    chip8->V[0xF] = (chip8->V[Y] > chip8->V[X]) ? 1 : 0;
                    chip8->V[X]   = chip8->V[Y] - chip8->V[X];
                    break;
                case 0xE:
                    chip8->V[0xF] = (chip8->V[X] >> 7) & 0x1;
                    chip8->V[X] <<= 1;
                    break;
                default:
                    fprintf(stderr, "unknown 0x8000 opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        case 0x9000:
            if (chip8->V[X] != chip8->V[Y]) chip8->pc += 2;
            break;

        case 0xA000:
            chip8->I = nnn;
            break;

        case 0xB000:
            chip8->pc = nnn + chip8->V[0];
            break;

        case 0xC000:
            chip8->V[X] = (rand() % 256) & kk;
            break;

        case 0xD000: {
            uint8_t x_loc = chip8->V[X];
            uint8_t y_loc = chip8->V[Y];
            chip8->V[0xF] = 0;

            for (int row = 0; row < n; row++) {
                uint8_t sprite = chip8->memory[chip8->I + row];
                for (int col = 0; col < 8; col++) {
                    if ((sprite & (0x80 >> col)) == 0) continue;

                    int sx    = (x_loc + col) % SCREEN_WIDTH;
                    int sy    = (y_loc + row) % SCREEN_HEIGHT;
                    int index = sx + (sy * SCREEN_WIDTH);

                    if (chip8->gfx[index]) chip8->V[0xF] = 1;
                    chip8->gfx[index] ^= 1;
                }
            }
            chip8->draw_flag = 1;
            break;
        }

        case 0xE000:
            switch (kk) {
                case 0x9E:
                    if (chip8->keypad[chip8->V[X]]) chip8->pc += 2;
                    break;
                case 0xA1:
                    if (!chip8->keypad[chip8->V[X]]) chip8->pc += 2;
                    break;
                default:
                    fprintf(stderr, "unknown 0xE000 opcode: 0x%04X\n", opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07:
                    chip8->V[X] = chip8->delay_timer;
                    break;
                case 0x0A: {
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
                    chip8->I = FONT_ADDR + (chip8->V[X] & 0x0F) * 5;
                    break;
                case 0x33: {
                    uint8_t val = chip8->V[X];
                    chip8->memory[chip8->I]     = val / 100;
                    chip8->memory[chip8->I + 1] = (val / 10) % 10;
                    chip8->memory[chip8->I + 2] = val % 10;
                    break;
                }
                case 0x55:
                    for (int i = 0; i <= X; i++)
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    break;
                case 0x65:
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