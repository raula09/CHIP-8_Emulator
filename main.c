#include <stdio.h>
#include "display.h"
#include "time.h"
int main(int argc, char *argv[]) {
    Chip8 chip8;
    Display display;

    init_chip8(&chip8);
    load_rom(&chip8, argv[1]);
    display_init(&display, 10);

    SDL_Event event;
    int running = 1;

    srand(time(NULL));

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            display_handle_input(&chip8, &event);
        }

        for (int i = 0; i < 10; i++)
            emulate_cycle(&chip8);

        if (chip8.delay_timer > 0) chip8.delay_timer--;
        if (chip8.sound_timer > 0) {
            chip8.sound_timer--;
            if (chip8.sound_timer == 0) printf("\a");
        }

        if (chip8.draw_flag) {
            display_draw(&display, &chip8);
            chip8.draw_flag = 0;
        }

        SDL_Delay(16);
    }

    display_destroy(&display);
    return 0;
}