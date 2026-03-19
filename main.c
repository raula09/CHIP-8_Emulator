#include <stdio.h>
#include "chip8.h"
#include "display.h"
#include <time.h>

int main(int argc, char *argv[]) {
    Chip8 chip8;
    Display display;

    init_chip8(&chip8);
    load_rom(&chip8, argv[1]);      // rom path comes from command line
    display_init(&display, 10);     // scale factor 10 gives us 640x320 window

    SDL_Event event;
    int running = 1;

    srand(time(NULL));  // seed rand for opcode 0xC000

    while (running) {
        // handle events first so input is always fresh
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            display_handle_input(&chip8, &event);
        }

        // run 10 cycles per frame to get roughly 600Hz cpu speed
        for (int i = 0; i < 10; i++)
            emulate_cycle(&chip8);

        // timers tick down at 60Hz, once per frame
        if (chip8.delay_timer > 0) chip8.delay_timer--;
        if (chip8.sound_timer > 0) {
            chip8.sound_timer--;
            if (chip8.sound_timer == 0) printf("\a");  // beep when it hits zero
        }

        // only redraw when something actually changed
        if (chip8.draw_flag) {
            display_draw(&display, &chip8);
            chip8.draw_flag = 0;
        }

        SDL_Delay(16);  // cap at ~60fps
    }

    display_destroy(&display);
    return 0;
}