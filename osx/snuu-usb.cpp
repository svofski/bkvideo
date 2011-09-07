#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "SDL.h"
#include <GL/gl.h>
#include <GL/glu.h>

#include "customusb.h"
#include "colores.h"
#include "pantalla.h"
#include "frame.h"

SDLCanvas canvas;

const int Width = 512,
          Height = 256; // 318

int WindowW, WindowH;

int fullscreen = 0;

GLuint magFilter = GL_LINEAR;

#define min(x,y) ((x)<(y)?(x):(y))

void initViewport(int w, int h, int fullscreen);

void render(SDL_Surface* screen, SDLCanvas* canvas, FrameBuffer* fb) {   
    // Lock surface if needed
    if (SDL_MUSTLOCK(screen))
        if (SDL_LockSurface(screen) < 0) 
            return;

    // Ask SDL for the time in milliseconds
    int tick = SDL_GetTicks();

    canvas->drawScreen((uint32_t*) fb->getBytes());

    // Unlock if needed
    if (SDL_MUSTLOCK(screen)) 
        SDL_UnlockSurface(screen);

    // Tell SDL to update the whole screen
    SDL_UpdateRect(screen, 0, 0, Width, Height);    
}

void glrender(SDL_Surface* screen, SDLCanvas* canvas, FrameBuffer* fb) {   
    GLuint texture;

    canvas->drawScreen((uint32_t*) fb->getBytes());

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
 
    // Set the texture's stretching properties
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Edit the texture object's image data using the information 
    // SDL_Surface gives us
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 
                    canvas->getSurface()->w, 
                    canvas->getSurface()->h, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, 
                    canvas->getSurface()->pixels);

    // opengl render
    glBindTexture( GL_TEXTURE_2D, texture );
     
    glBegin( GL_QUADS );
        //Bottom-left vertex (corner)
        glTexCoord2i( 0, 0 );
        glVertex3f( 0.f, 0.f, 0.0f );
     
        //Bottom-right vertex (corner)
        glTexCoord2i( 1, 0 );
        glVertex3f( WindowW, 0.f, 0.f );
     
        //Top-right vertex (corner)
        glTexCoord2i( 1, 1 );
        glVertex3f( WindowW, WindowH, 0.f );
     
        //Top-left vertex (corner)
        glTexCoord2i( 0, 1 );
        glVertex3f( 0.f, WindowH, 0.f );
    glEnd();     
    SDL_GL_SwapBuffers();
    glDeleteTextures(1, &texture);
}

void updateCaption(int detune, float fps) {
    char buf[512];
    sprintf(buf, "BK-0010 Video Capture: fps: %2.2f %c", 
            fps, detune ? '+' : '-');
    SDL_WM_SetCaption(buf, "BK-0010 video");
}

static void setup_opengl( int width, int height )
{
    glEnable( GL_TEXTURE_2D );
     
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
     
    glViewport(0, 0, width, height);
     
    glClear( GL_COLOR_BUFFER_BIT );
     
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
     
    glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
     
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

void initViewport(int w, int h, int fullscreen) {
    if (fullscreen) {
        w = 1024;
        h = 768;
        magFilter = GL_NEAREST;
    } else {
        magFilter = GL_LINEAR;
    }

    SDL_Surface* s = SDL_SetVideoMode(w, h, 32, 
                     SDL_OPENGL|SDL_RESIZABLE|
                     (fullscreen?SDL_FULLSCREEN:0));

    if (fullscreen) {
        fprintf(stderr, "w,h=%d,%d\n", s->w, s->h);
        w = s->w;
        h = s->h;
    }

    WindowW = w;
    WindowH = h;

    setup_opengl(w, h);
}

SDL_Surface* initSDLShit() {
    SDL_Surface* screen;

    // Initialize SDL's subsystems - in this case, only video.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
#if 0
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
#endif
    initViewport(Width, Height, fullscreen);

    atexit(SDL_Quit);

    // create a drawing surface
    screen = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 
                                  32, 
                                  0x000000ff,0x0000ff00,
                                  0x00ff0000,0xff000000);
    return screen;
}

void freeSDLShit(SDL_Surface** screen) {
    SDL_FreeSurface(*screen);
    *screen = 0;
}

void mainLoop() {
    int quit = 0;
    int frameResult = 0;
    int tuning = 0;
    float fps;
    time_t time_end, time_start, timestamp;
    SDL_Surface* screen;

    screen = initSDLShit();

    updateCaption(0, 0);
    FrameReceiver receiver;
    receiver.init(16*4*Height);

    canvas.init(screen, Width, Height);

    for(time_start = timestamp = SDL_GetTicks(); !quit;) {
        if (receiver.isConnected()) {
            frameResult = receiver.receiveFrame();
        } else {
            receiver.connect();
            if (!receiver.isConnected()) {
                receiver.makeRandomFrame();
                usleep(35000);
                frameResult = 1;
            }
        }

        switch (frameResult) {
        case 1:
            //render(screen, &canvas, &receiver);
            glrender(screen, &canvas, &receiver);
            receiver.nextFrame();
            tuning = receiver.diffLastFrame();
            if (receiver.frameCount() % 25 == 0) {
                fps = 25000.0/(SDL_GetTicks() - timestamp);
                timestamp = SDL_GetTicks();
            }
            updateCaption(tuning, fps);
            break;
        case 2:
        case 3:
        case 4:
            fprintf(stderr, "Incomplete frame\n");
            receiver.nextFrame();
            break;
        default:
            break;
        }

        // Poll for events, and handle the ones we care about.
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            switch (event.type) 
            {
            case SDL_KEYDOWN:
                fprintf(stderr, "key: %s %d\n", 
                        SDL_GetKeyName(event.key.keysym.sym), 
                        event.key.keysym.sym);
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = 1;
                    break;
                case 'c':
                    canvas.toggleColorMode();
                    break;
                case 'q':
                    magFilter = GL_NEAREST;
                    break;
                case 'w':
                    magFilter = GL_LINEAR;
                    break;
                case 'e':
                    magFilter = GL_NEAREST_MIPMAP_NEAREST;
                    break;
                case 'r':
                    magFilter = GL_NEAREST_MIPMAP_LINEAR;
                    break;
                case 't':
                    magFilter = GL_LINEAR_MIPMAP_NEAREST;
                    break;
                case 'y':
                    magFilter = GL_LINEAR_MIPMAP_LINEAR;
                    break;
                case 'f':
                    initViewport(WindowW, WindowH, fullscreen=!fullscreen);
                    break;
                default:
                    receiver.sendCommand(event.key.keysym.sym);
                }
                break;
            case SDL_VIDEORESIZE:
                initViewport(event.resize.w, event.resize.h, fullscreen);
                break;
            case SDL_QUIT:
                fprintf(stderr, "joder, me voy\n");
                quit = 1;
                break;
            }
        }
    }

    time_end = SDL_GetTicks();

    receiver.waitForLastTransfer();
    receiver.disconnect();

/*
    fprintf(stderr, 
        "Time spent: %d average FPS: %d\n", 
        time_end - time_start, 
        1000*receiver.frameCount()/(time_end-time_start));
*/
    freeSDLShit(&screen);
}


// Entry point
int main(int argc, char *argv[])
{
    mainLoop();

    return 0;
}

