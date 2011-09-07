#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "SDL_ttf.h"
#include "SDL.h"

#include "serialski.h"

SDL_Surface *screen;

const int Width = 512,
          Height = 318;

TTF_Font *font = 0;

const char* Gruuu = "Snuu";
const SDL_Color colorSnuu = {1, 0, 0};
const SDL_Color colorGruu = {255, 0, 0};

uint32_t colorWhiteInt;
uint32_t colorBlackInt;
uint32_t colorYellowInt;

void putpixel(int x, int y, int color)
{
    unsigned int *ptr = (unsigned int*)screen->pixels;
    int lineoffset = y * (screen->pitch / 4);
    ptr[lineoffset + x] = color;
}

#define PITCH (screen->pitch / 4)

int initText() {
    fprintf(stderr, "initText()\n");
    font=TTF_OpenFont("BPreplayBold.otf", 64);
    if(!font) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
        return -1;
    }
    fprintf(stderr, "initText() done\n");
}

void drawGruu() {
    int textWidth, textHeight;
    
    TTF_SizeText(font, Gruuu, &textWidth, &textHeight);
    SDL_Rect  targetRect = {Width/2 - textWidth/2, 100, 320, 240};

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, Gruuu, colorGruu);
    if (textSurface != 0) {
        SDL_BlitSurface(textSurface, NULL, screen, &targetRect);
        SDL_FreeSurface(textSurface);
    }
}

void drawScreen(uint32_t* stuff) {
    int ofs = 0;

    ofs = 16;

    for (int line = 1; line < Height; line++) {
        //fprintf(stderr, "line #%d ofs:%x\n", line, ofs);
        for (int word = 0; word < Width/32; word++) {
            uint32_t bits32 = stuff[ofs];
            for (int bit = 0; bit < 32; bit++) {
                putpixel(word*32 + bit, line, ((bits32 & 0x80000000) == 0) ? colorBlackInt : colorWhiteInt);
                bits32 <<= 1;
            }
            ofs++;
        }
    }

    for (int x = 0; x < Width; x += 8) {
        for (int y = 0; y < Height; y += 8) {
            putpixel(x, y, colorYellowInt);
        }
    }

    //fprintf(stderr, "screen size in dwords: %d\n", ofs);
}

void init() {
    colorWhiteInt = SDL_MapRGB(screen->format, 255, 255, 255);
    colorBlackInt = SDL_MapRGB(screen->format, 0, 0, 0);
    colorYellowInt = SDL_MapRGB(screen->format, 200, 200, 0);

    fprintf(stderr, "White is %x\n", colorWhiteInt);
    initText();
}


#define min(x,y) ((x)<(y)?(x):(y))

int openSerial(int argc, char *argv[]) {
    // open serial
    char        bsdPath[256];

    if (argc > 1) {
        strncpy(bsdPath, argv[1], 256);
    } else {
        strcpy(bsdPath, "/dev/tty.usbserial-A5004H5Q");
    }

    return OpenSerialPort(bsdPath);
}

void closeSerial(int handle) {
    CloseSerialPort(handle);
}

class Expectador {
    int matchoffset;
    const char *match;
    int matchsize;
    int handle;

    public: 
    Expectador(int shandle) : handle(shandle) {}

    void init(const char* match) {
        matchoffset = 0;
        this->match = match;
        matchsize = strlen(match);
    }

    int expect() {
        uint8_t c;

        if (read(handle, &c, 1) == 1) {
            putchar('[');
            putchar(c > 'a' && c < 'z' ? c : '.');
            putchar(']');
            if (c == match[matchoffset]) {
                matchoffset++;
                if (matchoffset == matchsize) {
                    matchoffset = 0;
                    printf("expectador: match\n");
                    return 1;
                } 
            } else {
                printf("expectador: mismatch\n");
                matchoffset = 0;
            }
        } 

        return 0;
    }
};

class FrameBuffer {
    public:
        virtual uint8_t* getBytes() = 0; 
};

class FrameReceiver : public FrameBuffer {
    int handle;
    int offset;
    int framebytes;
    uint8_t buffer[65536];

    public:

    FrameReceiver(int serialHandle) : handle(serialHandle) {}

    void init(int framebytes) {
        offset = 0;
        this->framebytes = framebytes;
    }

    int receiveFrame() {
        int bytesread = read(handle, buffer + offset, framebytes - offset);
        offset += bytesread;
        return offset == framebytes;
    }

    virtual uint8_t* getBytes() {
        return buffer;
    }
};

void render(int nframe, FrameBuffer* fb) {   
#if 0
    // Lock surface if needed
    if (SDL_MUSTLOCK(screen))
        if (SDL_LockSurface(screen) < 0) 
            return;

    // Ask SDL for the time in milliseconds
    int tick = SDL_GetTicks();
#endif
    drawScreen((uint32_t*) fb->getBytes());
#if 0
    // Unlock if needed
    if (SDL_MUSTLOCK(screen)) 
        SDL_UnlockSurface(screen);
#endif

    // Tell SDL to update the whole screen
    SDL_UpdateRect(screen, 0, 0, Width, Height);    
}

void mainLoop2(FrameBuffer* fb);

void initSDLShit() {
    // Initialize SDL's subsystems - in this case, only video.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    // Attempt to create a 640x480 window with 32bit pixels.
    screen = SDL_SetVideoMode(Width, Height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

    fprintf(stderr, "Screen init ok\n");

    // If we fail, return error.
    if ( screen == NULL ) 
    {
        fprintf(stderr, "Unable to set 640x480 video: %s\n", SDL_GetError());
        exit(1);
    }

    init();

    fprintf(stderr, "after init()\n");

}


void mainLoop(int serial) {
    int nframes = 0;
    int quit = 0;
    time_t time_end, time_start;

    int state = 0;

    Expectador expectador(serial);
    FrameReceiver receiver(serial);

    expectador.init("peli");

    initSDLShit();

    for(time_start = time(0);!quit;nframes++) {
        switch (state) {
        case 0:
            if (expectador.expect()) {
                printf("received frame header\n");
                state = 1;
                receiver.init(16*4*318);
            }
            break;
        case 1:
            if (receiver.receiveFrame()) {
                state = 0;
                printf("received complete frame\n");
                //quit = 1;
                render(0, &receiver);
            }
            break;
        }

        // Poll for events, and handle the ones we care about.
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
            case SDL_KEYDOWN:
                fprintf(stderr, "key: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                break;
            case SDL_QUIT:
                fprintf(stderr, "joder, me voy\n");
                quit = 1;
                break;
            }
        }
    }

    time_end = time(0);

    fprintf(stderr, "Time spent: %d average FPS: %d\n", time_end - time_start, nframes/(time_end-time_start));

    closeSerial(serial); 

    fprintf(stderr, "Frame received and serial port is now closed. Prepare to die anyway.\n");

    fprintf(stderr, "fb: %x %x\n", receiver.getBytes()[0], receiver.getBytes()[1]);

    //mainLoop2(&receiver);
}

void mainLoop2(FrameBuffer* fb) {
    int quit = 0;

    initSDLShit();

    for (;!quit;) {
        // Render stuff
        render(0, fb);

        // Poll for events, and handle the ones we care about.
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
            case SDL_KEYDOWN:
                fprintf(stderr, "key: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                break;
            case SDL_QUIT:
                fprintf(stderr, "joder, me voy\n");
                quit = 1;
                break;
            }
        }
    }
}


// Entry point
int main(int argc, char *argv[])
{
    int serial;

    if ((serial = openSerial(argc, argv)) == -1) {
        fprintf(stderr, "Cannot open serial device: %s", argv[1]);
    } else {
        fprintf(stderr, "Serial port opened\n");
    }

    mainLoop(serial);

    return 0;
}

