#pragma once

#include "SDL.h"

class SDLCanvas {
public:
    void init(SDL_Surface* screen, int width, int height) {
        this->screen = screen;
        this->colorMode = 0;
        this->width = width;
        this->height = height;
        colors.init(screen);
    }

    void toggleColorMode() { colorMode = !colorMode; }

    void drawScreen(uint32_t* stuff) {
        int ofs = 0;

        putNextPixel(0, 1);

        for (int line = 0; line < height; line++) {
            for (int word = 0; word < width/32; word++) {
                uint32_t bits32 = stuff[ofs];
                if (colorMode) {
                    for (int bit = 0; bit < 32; bit+=2) {
                        uint32_t c = colors.BK_Palette[(bits32 & 0xc0000000)>>30];
                        putNextPixel(c, 0);
                        putNextPixel(c, 0);

                        bits32 <<= 2;
                    }
                } else {
                    for (int bit = 0; bit < 32; bit++) {
                        uint32_t c = ((bits32 & 0x80000000)==0) ?
                                      colors.black : colors.white;
                        putNextPixel(c, 0);
                        bits32 <<= 1;
                    }
                }
                ofs++;
            }
        }
    #ifdef DRAWGRID
        for (int x = 0; x < Width; x += 8) {
            for (int y = 0; y < Height; y += 8) {
                putpixel(x, y, colorYellowInt);
            }
        }
    #endif
    }

    SDL_Surface* getSurface() { return screen; }
private:
    SDL_Surface* screen;
    int colorMode;
    Colors colors;
    int width, height;

    void putpixel(int x, int y, int color) {
        unsigned int *ptr = (unsigned int*)screen->pixels;
        int lineoffset = y * (screen->pitch / 4);
        ptr[lineoffset + x] = color;
    }

    uint32_t* putNextPixel(uint32_t color, uint32_t init) {
        static uint32_t* ptr;

        if (init) {
            ptr = (uint32_t *) screen->pixels;
        } else {
            *ptr++ = color;
        }
        return ptr;
    } 
};
