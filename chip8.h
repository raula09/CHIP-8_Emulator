#ifndef CHIP8_H
#define CHIP8_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define MEMORY_SIZE 4096
#define NUM_REGISTERS 16
#define STACK_SIZE 16
#define NUM_KEYS 16
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define START_ADDRESS 0x200

typedef struct {
    // 4KB
    uint8_t memory[MEMORY_SIZE];

    // 16 registers
    uint8_t V[NUM_REGISTERS];

    //INDEX register
    uint16_t I;

    // program counter
    uint16_t pc;

    // stack and it's pointers
    uint16_t stack[STACK_SIZE];
    uint8_t sp;

    //timers
    uint8_t delay_timer;
    uint8_t sound_timer;

    // Graphics buffer (64x32 pixels)
    uint8_t gfx[SCREEN_WIDTH * SCREEN_HEIGHT];

    //Keypad state (16 keys)
    uint8_t keypad[NUM_KEYS];

    //current opcode
    uint16_t opcode;
} Chip8;

//init the chip-8 system
void init_chip8(Chip8 *c);

//load rom into memory
void load_rom(Chip8 *c, const char *filename);

//emulate one cpu cycle
void emulate_cycle(Chip *c);

//execute a single opcode
void execute_upcode(Chip8 *c);

#endif
