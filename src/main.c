#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include "types.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <rom.gb>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("GB Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GB_SCREEN_W * 3, GB_SCREEN_H * 3, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "window: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, GB_SCREEN_W, GB_SCREEN_H);

    /* 160x144 framebuffer, ARGB8888. Placeholder: solid green. */
    u32 framebuffer[GB_SCREEN_W * GB_SCREEN_H];
    for (int i = 0; i < GB_SCREEN_W * GB_SCREEN_H; i++)
        framebuffer[i] = 0xFF9BBC0F;

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        SDL_UpdateTexture(tex, NULL, framebuffer, GB_SCREEN_W * sizeof(u32));
        SDL_RenderClear(ren);
        SDL_RenderCopy(tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
