//#define BAUDRATE 1500000

extern "C" {
// Function prototypes
int OpenSerialPort(const char *bsdPath);
void CloseSerialPort(int fileDescriptor);
}
