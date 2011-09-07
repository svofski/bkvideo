#pragma once

class SyncDetector {
private:
    int statevar;
public:
    SyncDetector() {}

    void init();
    void shutdown();
    void suspend();
    void resume();

    static uint32_t StartOfPixelsTime;
    static uint32_t CTInterruptCount;
    static uint32_t LineNumber;
    static uint32_t LineCount;
    static uint32_t FrameCount;
    static uint32_t LineState;
    static uint32_t LastFrameLineCount;
    static uint32_t DMAInterrupts;
};

extern SyncDetector Sync;
