#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include "chip8.h"

// holds all the SDL stuff we need to keep around
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;   // streaming texture we update every frame
} Display;

void display_init(Display *d, int scale);           // set up SDL window and renderer
void display_draw(Display *d, Chip8 *chip8);        // push gfx buffer to the screen
void display_handle_input(Chip8 *chip8, SDL_Event *event);  // map keys to chip8 keypad
void display_destroy(Display *d);                   // clean up SDL on exit

#endif