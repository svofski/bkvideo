#pragma once

#define WITH_PTHREADS

#include <stdio.h>
#include <pthread.h>
#include "customusb.h"

class FrameBuffer {
    public:
        virtual uint8_t* getBytes() = 0;
        virtual uint8_t* getLastBytes() = 0;
        virtual uint32_t getSize() const = 0;
};

void *connectDeviceT(void*);

class FrameReceiver : public FrameBuffer {
    int offset;
    int framebytes;
    int transferActive;
    int nframe;

    int connected;
    int connecting;
    pthread_t connectorThread;
    int threadCreated;

    uint8_t buffer[2][16384];

    public:

    FrameReceiver(void) : 
        connected(0), 
        connectorThread(0), connecting(0), threadCreated(0) {}

    void init(int framebytes) {
        this->offset = 0;
        this->framebytes = framebytes;
        this->transferActive = 0;
        this->nframe = 0;

        createThread();
    }

    void createThread() {
#ifdef WITH_PTHREADS
        if (!threadCreated) {
            pthread_attr_t  attr;

            connecting = 1;

            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&connectorThread, &attr, connectDeviceT, this);
            pthread_attr_destroy(&attr);

            threadCreated = 1;
        }
#endif
    }

    int isConnected() const { return connected; }
    void setConnected(int c) { connected = c; }
    
    void connectionThreadDone() { connecting = 0; }
    int isConnecting() const { return connecting; }

    int connect() {
#ifdef WITH_PTHREADS
        if (connecting == 0) {
            setConnected(1);
            setupTransfer(getBytes(), getSize());
            sendUSBCommand(1001);
            fprintf(stderr, "connect(): Connected\n");
        }
#else
        if (connectDevice() == 0) {
            setConnected(1);
            setupTransfer(getBytes(), framebytes);
            fprintf(stderr, "The transfer is set up: %d bytes\n", framebytes);
        }
        return connected;
#endif
    }

    void disconnect() {
        disconnectDevice(); 
        setConnected(0);
        connecting = 1;
    }

    void sendCommand(uint32_t command) {
        if (!connected) 
            return;

        sendUSBCommand(command);
        switch (command) {
        case 48:    // '0', reset
#if 0
            usleep(100000);
            sendUSBCommand(command);
            sendUSBCommand(command);
            usleep(1000);
            sendUSBCommand(command);
            cancelUSBTransfer();
            usleep(10000);
            init(framebytes);
#else   
            sendUSBCommand(1000);
            //sleep(1);
            //cancelUSBTransfer();
            //init(framebytes);
            //sendUSBCommand(1001);
#endif
            break;
        case 39:
            cancelUSBTransfer();
            sendUSBCommand(1001);
            break;
        }
    }

    void nextFrame() {
        transferActive = 0;
        nframe++;
    }

    int receiveFrame() {
        int status;

        if (!transferActive) {
            status = submitTransfer(getBytes());
            if (status > 0) {
                transferActive = 1;
            } else {
                // negative status means error
                // probably the device was disco'd
                fprintf(stderr, "disconnecting because submit failed\n");
                disconnect();
            }
        }
        return pollComplete();
    }

    void makeRandomFrame() {
        uint8_t* frame = getBytes();
        for (int i = getSize(); --i >= 0;) {
            frame[i] = (int8_t) rand();
        }
    }

    void waitForLastTransfer() {
        if (!connected)
            return;

        fprintf(stderr, "waitForLastTransfer()\n");
        sendUSBCommand(1000);
        for (;;) {
            int result = receiveFrame();
            switch(result) {
            case 0:
                break;
            case 1:
                nextFrame();
                break;
            default:
                freeUSBTransfer();
                return;
            }
            usleep(10000);
        }
    }

    virtual uint32_t getSize() const {
        return framebytes;
    }

    virtual uint8_t* getBytes() {
        return buffer[nframe & 1];
    }

    virtual uint8_t* getLastBytes() {
        return buffer[1 - (nframe & 1)];
    }

    virtual int32_t diffLastFrame() const {
        int i, diffs;

        for (i = 0, diffs = 0;
             i < sizeof(buffer[0]);
             diffs += (buffer[0][i] == buffer[1][i]) ? 0 : 1, i++);

        return diffs;
    }

    uint32_t frameCount() const { return nframe; }

private:
};

void *connectDeviceT(void* arg) {
    FrameReceiver* fr = (FrameReceiver *) arg; 

    for(;;) {
        if (fr->isConnecting()) {
            if (connectDevice() == 0) {
                fprintf(stderr, "thread: connected\n");
                fr->connectionThreadDone();
            }
        }
        sleep(1);
    }

    return NULL;
}


