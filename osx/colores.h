#pragma once

#include "SDL.h"

class Colors
{
public:
    uint32_t white;
    uint32_t black;
    uint32_t yellow;

    uint32_t red;
    uint32_t green;
    uint32_t blue;

    uint32_t BK_Palette[4]; 

    void init(SDL_Surface* screen) {
        white = SDL_MapRGB(screen->format, 255, 255, 255);
        black = SDL_MapRGB(screen->format, 0, 0, 0);
        yellow = SDL_MapRGB(screen->format, 64, 64, 24);
        red = SDL_MapRGB(screen->format, 255, 0, 0);
        green = SDL_MapRGB(screen->format, 0, 255, 0);
        blue = SDL_MapRGB(screen->format, 0, 0, 255);

        BK_Palette[0] = black;
        BK_Palette[1] = blue;
        BK_Palette[2] = green;
        BK_Palette[3] = red;
    };
};
