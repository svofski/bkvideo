#pragma once


extern "C" {
int connectDevice();
void disconnectDevice();
void setupTransfer(uint8_t* buffer, uint32_t size);
int submitTransfer(uint8_t* buffer);
void cancelUSBTransfer();
void freeUSBTransfer();
int pollComplete();
void sendUSBCommand(uint32_t command);
}
