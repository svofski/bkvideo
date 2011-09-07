#pragma once

#include <inttypes.h>
#include <p32xxxx.h>
#include <peripheral/int.h>

class SPIShifter {
    public:
        SPIShifter();

        void init();
        void shutdown();
        void start(uint32_t* bufptr, uint32_t wcount);
        void abort();
        void oversample(int x); // valid values 1, 2, 3

        inline int toggleCKP() const { 
            SPI1CONINV = 0x40; 
            return SPI1CONbits.CKP;
        }

        inline int toggleCKE() const {
            SPI1CONINV = 0x100;
            return SPI1CONbits.CKE;
        }
};

extern SPIShifter Shifter;
