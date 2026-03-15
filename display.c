#include "display.h"

static SDL_Scancode keymap[16] = {
    SDL_SCANCODE_X,    // 0x0
    SDL_SCANCODE_1,    // 0x1
    SDL_SCANCODE_2,    // 0x2
    SDL_SCANCODE_3,    // 0x3
    SDL_SCANCODE_Q,    // 0x4
    SDL_SCANCODE_W,    // 0x5
    SDL_SCANCODE_E,    // 0x6
    SDL_SCANCODE_A,    // 0x7
    SDL_SCANCODE_S,    // 0x8
    SDL_SCANCODE_D,    // 0x9
    SDL_SCANCODE_Z,    // 0xA
    SDL_SCANCODE_C,    // 0xB
    SDL_SCANCODE_4,    // 0xC
    SDL_SCANCODE_R,    // 0xD
    SDL_SCANCODE_F,    // 0xE
    SDL_SCANCODE_V     // 0xF
};

void display_init(Display *d, int scale) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    d->window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * scale,
        SCREEN_HEIGHT * scale,
        SDL_WINDOW_SHOWN
    );
    d->renderer = SDL_CreateRenderer(d->window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(d->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    d->texture = SDL_CreateTexture(
        d->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
}

void display_draw(Display *d, Chip8 *chip8) {
    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = chip8->gfx[i] ? 0xFFFFFFFF : 0x000000FF;
    }

    SDL_UpdateTexture(d->texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(d->renderer);
    SDL_RenderCopy(d->renderer, d->texture, NULL, NULL);
    SDL_RenderPresent(d->renderer);
}

void display_handle_input(Chip8 *chip8, SDL_Event *event) {
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        for (int i = 0; i < 16; i++) {
            if (event->key.keysym.scancode == keymap[i]) {
                chip8->keypad[i] = (event->type == SDL_KEYDOWN) ? 1 : 0;
                break;
            }
        }
    }
}

void display_destroy(Display *d) {
    SDL_DestroyTexture(d->texture);
    SDL_DestroyRenderer(d->renderer);
    SDL_DestroyWindow(d->window);
    SDL_Quit();
}