#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include "chip8.h"

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
} Display;

void display_init(Display *d, int scale);
void display_draw(Display *d, Chip8 *chip8);
void display_handle_input(Chip8 *chip8, SDL_Event *event);
void display_destroy(Display *d);

#endif