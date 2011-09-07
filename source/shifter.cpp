/*

Use SPI slave to continuously shift in video bits and stash them in RAM using DMA.
Use another SPI module as frame master to generate "pixelclock".
An external wire is needed to connect the SCK of SPI master to SPI slave.

*/

#include <inttypes.h>
#include <p32xxxx.h>
#include <plib.h>
#include <peripheral/int.h>

#include "shifter.h"

SPIShifter::SPIShifter() 
{
}

void SPIShifter::init() {
    uint32_t dummy;

    // initialize SPI1 as slave
    // no interrupts enabled: DMA will sniff it
    // Receive data available interrupts,  signalled by SPI1RXIF (IFS0<25>)
    // Check: "full by one-half" in Enhanced Buffering Mode

    SPI1CON = 0;                // stop and reset SPI1
    dummy = SPI1BUF;            // clear throat 
    SPI1STATCLR = 0x40;         // clear overflow
    SPI1CONbits.MODE32 = 1;     // 32-bit mode
    SPI1CONbits.SSEN   = 0;     // no SS 
    SPI1CONbits.ON     = 0;     // it's not yet ON
    SPI1CONbits.SIDL   = 0;     // continue operation in idle mode
    SPI1CONbits.CKP = 0;
    SPI1CONbits.CKE = 1;


    // Initialize SPI2 as frame master to provide continuous clock
    SPI2STATCLR = 0x40;
    SPI2CONCLR = -1;
    SPI2CONbits.FRMEN = 1;      // framed mode
    SPI2CONbits.MSTEN = 1;      // master, poop clock
    SPI2CONbits.DISSDO =1;      // disable SDO2
    SPI2CONbits.SIDL   = 0;     // continue operation in idle mode
    
    oversample(1);

    // setup comparator 2
    CM2CON = 0;
    CM2CONbits.COE = 1;
    CM2CONbits.ON = 1;
    CM2CONbits.CCH = 3;         // Comparator Negative Input connected to IVref
                                //            Positive Input connected to C2IN+ pin
#ifdef USE_DMA
//    SPI1CONbits.ENHBUF = 1;
//    SPI1CONbits.SRXISEL = 2;
    ConfigIntSPI1(SPI_RX_INT_EN);

    DmaChnOpen(DMA_CHANNEL1, DMA_CHN_PRI0, DMA_OPEN_AUTO); // AUTO -- same thing

    DmaChnSetEventControl(DMA_CHANNEL1,
        (DmaEvCtrlFlags)(DMA_EV_START_IRQ_EN|DMA_EV_START_IRQ(_SPI1_RX_IRQ))); 

    DmaChnClrEvFlags(DMA_CHANNEL1, DMA_EV_ALL_EVNTS);
    DmaChnSetEvEnableFlags(DMA_CHANNEL1, (DmaEvFlags)(DMA_EV_BLOCK_DONE));
    mDmaChnSetIntPriority(1, 1, 1);
    mDmaChnIntEnable(1);
#endif
}

void SPIShifter::oversample(int x) {
    switch (x) {
    case 1:
        SPI2BRG = 2;                // Fpb / 4: 12 MHz
        break;
    case 2:
        SPI2BRG = 1;                // Fpb / 3: 24 MHz
        break;
    case 3:
        SPI2BRG = 0;                // Fpb / 2: 36 MHz, 3x oversample
        break;
    }
}

void SPIShifter::shutdown() {
}

void SPIShifter::start(uint32_t* bufptr, uint32_t wcount) 
{
    int x;

#ifdef USE_DMA
    INTClearFlag(INT_SPI1);
    DmaChnClrEvFlags(DMA_CHANNEL1, DMA_EV_ALL_EVNTS);
    DmaChnSetTxfer(DMA_CHANNEL1, (void*) &SPI1BUF, bufptr, 4, wcount, 4/**2*/);
    DmaChnEnable(DMA_CHANNEL1);
    SPI1CONbits.ON = 1;
    SPI2CONbits.ON = 1;
#else
    SPI1CONbits.ON = 1;
    SPI2CONbits.ON = 1;

    for (x = 16; --x >= 0;) {
        while(SPI1STATbits.SPIRBF == 0); 
        *bufptr++ = SPI1BUF;
    }

    SPI1CONbits.ON = 0;
    SPI2CONbits.ON = 0;
#endif
}

void SPIShifter::abort()
{
    int x;
    DCH1ECONbits.CABORT = 1;            // abort transfer programmatically
    SPI1STATCLR = 0x40;         // clear overflow
    SPI1CONbits.ON = 0;
    SPI2CONbits.ON = 0;
    x = SPI1BUF; 
}

SPIShifter Shifter;

