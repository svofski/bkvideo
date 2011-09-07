#include <stdio.h>
#include <inttypes.h>
#include <p32xxxx.h>

static volatile uint32_t delay_ctr;

#include <HardwareSerial.h>
#include <wiring.h>

#include "shifter.h"
#include "sync.h"
#include "frame.h"

volatile int USBInterruptCount = 0;

volatile int EnableSending = 0;

void SwitchClock() {
    // First, switch to fast RC oscillator
    SYSKEY = 0x0; // write invalid key to force lock
    SYSKEY = 0xAA996655; // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA; // Write Key2 to SYSKEY

    OSCConfig(OSC_FRC, 0, 0, 0); // set CPU clock to FRC

    // input divider is magically remembered to be 2

#if F_CPU == 72000000L
    OSCConfig(OSC_POSC_PLL, OSC_PLL_MULT_18, OSC_PLL_POST_1, 0);
#elif F_CPU == 96000000L
    OSCConfig(OSC_POSC_PLL, OSC_PLL_MULT_24, OSC_PLL_POST_1, 0);
#elif F_CPU == 80000000L
    OSCConfig(OSC_POSC_PLL, OSC_PLL_MULT_20, OSC_PLL_POST_1, 0);
#endif

    __PIC32_pbClk = SYSTEMConfigPerformance(F_CPU);

    SYSKEY = 0;
}

extern "C" {
void USBDeviceAttach(void);

void InitializeSystem(void);
int ProcessIO(int start, uint32_t* command);
void USBDeviceTasks(void);
void USBCBSendResume(void);
void BlinkUSBStatus(void);
void USBUserInit(uint8_t* buffer, uint32_t size);
void InitializeUSART(void);
void putcUSART(char c);
unsigned char getcUSART ();

void usb_initialize(void);
void usb_device_wait(void);
}

extern volatile int USBDeviceState;

void usbstuff() {
    InitializeSystem();

    USBUserInit((uint8_t*)FrameBuffer.GetLinePtr(42), 
                FrameBuffer.LineSizeInBytes*256);

    Serial.println("Calling USBDeviceAttach()");Serial.println(USBInterruptCount);
    USBDeviceAttach();
    Serial.print("After:"); Serial.println(USBInterruptCount);
}

// led on RA3, PORTA

extern int screen_state;

inline void USBInterruptEnable(int enable) {
    IEC1bits.USBIE = enable;
}

void handleCommand(uint32_t command, int32_t* skipFrame) {
    switch (command) {
    case 0:  break;
    case 1000:
        EnableSending = 0;
        Serial.print("EnableSending="); Serial.println(EnableSending);
        break;
    case 1001: // '0', reset
        //*skipFrame = 300;
        EnableSending = 1;
        Serial.print("EnableSending="); Serial.println(EnableSending);
        break;

    case 49:
    case 50: // '1', '2', '3'
    case 51:
        Shifter.oversample(command - 48);
        break;
    case 275: // left
        Sync.StartOfPixelsTime--;
        Serial.print("Start of pixels: "); 
        Serial.println(Sync.StartOfPixelsTime);
        break;
    case 276: // right
        Sync.StartOfPixelsTime++;
        Serial.print("Start of pixels: "); 
        Serial.println(Sync.StartOfPixelsTime);
        break;
    case 273: // up
        Serial.print("CKP: ");
        Serial.println(Shifter.toggleCKP());
        break;
    case 274: // down
        Serial.print("CKE: ");
        Serial.println(Shifter.toggleCKE());
        break;
    default:
        Serial.println(command);
    }
}

volatile int dummy;

int main() 
{
    int line_number = 0;
    uint32_t command;
    int skipFrame;

    // initialize wiring stuff
    init();

    // switch clock to 72 MHz
    SwitchClock();

    TRISA = 0;

    Serial.begin(BAUDRATE);
    Serial.println("Hooray");

    // Init usb stuff
    usbstuff();
    Serial.println("usbstuff() done");

    Shifter.init();
    Sync.init(); 
    mConfigIntCoreTimer(0);

    int io = 0;
    skipFrame = 0;
    delay_ctr = 0;
    EnableSending = 0;
    for(;;) {
        command = 0;
        
        if (io == 0 && Sync.LineNumber == 298 /*317*/) {
            Sync.suspend();
            for (dummy = 0; dummy < 2; dummy++);
            io = 1;
            USBInterruptEnable(1);
            ProcessIO(EnableSending, &command);
        }
        else 
        if (io) {
            if (ProcessIO(0, &command) == 0) {
                io = 0;
                USBInterruptEnable(0);
                Sync.resume();
            }
        }

        if (command) {
            handleCommand(command, &skipFrame);
        }

        if (++delay_ctr == 100000) {
            PORTAINV = 1 << 3;
            delay_ctr = 0;
        }

        if (!io) PowerSaveIdle();
    }


    return 0;
}
