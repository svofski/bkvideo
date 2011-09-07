#pragma once

#include <inttypes.h>
#include <p32xxxx.h>

#define LINE_SAMPLES    512
#define LINE_DWORDS     (LINE_SAMPLES/32)
#define LINE_BYTES      (LINE_DWORDS*4)
#define LINES_TOTAL     318

class Frame {
public:
    static const int LineSizeInBytes = LINE_SAMPLES/32*4;
    static const int SizeInBytes = LineSizeInBytes * LINES_TOTAL;
    static const int SizeInDwords = LineSizeInBytes/4 * LINES_TOTAL;

    Frame() {}

    uint32_t* NewFrame() {
        return &buffer[bufofs = 0];
    }

    uint32_t* NewLine() {
        int ofs = bufofs;
        bufofs += LINE_DWORDS;
        return &buffer[ofs]; 
    }

    uint32_t* GetCurrentLinePtr() { return &buffer[bufofs]; }

    uint32_t* GetLinePtr(uint32_t line_number) { return &buffer[line_number*LINE_DWORDS]; }

private:
    uint32_t buffer[16000];     // 50 words per line, 320 lines
    int32_t bufofs;
};

extern Frame FrameBuffer;
