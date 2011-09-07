// Includes
#include <stdio.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include <libusb-1.0/libusb.h>

//Modify this value to match the VID and PID in your USB device descriptor.//Modify this value to match the VID and PID in your USB device descriptor.
#define MY_VID 0x04D8
#define MY_PID 0x0204

/* the device's endpoints */
#define EP_IN 0x81
#define EP_OUT 0x01

#define CONNECTED  0x01

#define EP_SIZE 255
#define TRANSFER_SIZE 49152

int Connection_Status = 0;

// Global Variables
libusb_device_handle *MyLibusbDeviceHandle = NULL;  /* the device handle */

/* The Below Function Searches for a device with given VID and PID.
It return a hanlde to the device if the device is found in the Bus.*/
libusb_device_handle *open_dev(void)
{
    return libusb_open_device_with_vid_pid(NULL, MY_VID, MY_PID);
}

int connectDevice() {
    if ( Connection_Status != CONNECTED)  {
        libusb_init(NULL); 

        if(!(MyLibusbDeviceHandle = open_dev())) {
            fprintf(stderr, "open_dev() failed\n");
            return -1;
        }
        
        // Sets the Active configuration of the device
        if (libusb_set_configuration(MyLibusbDeviceHandle, 1) < 0) {

            fprintf(stderr, "usb_set_configuration() failed\n");
            libusb_close(MyLibusbDeviceHandle);
            return -2;
        }

        // claims the interface with the Operating System
        if(libusb_claim_interface(MyLibusbDeviceHandle, 0) < 0)  
        {
            fprintf(stderr, "usb_claim_interface() failed\n");
            //Closes a device opened since the claim interface is failed.
            libusb_close(MyLibusbDeviceHandle);
            return -3;
        }

        // Everything went better than expected
        Connection_Status = CONNECTED; 
    }

    return 0;
}

void disconnectDevice() {
    if ( Connection_Status == CONNECTED) {
        //The following functiom releases a previously claimed interface
        libusb_release_interface(MyLibusbDeviceHandle, 0);
        //closes a device opened
        libusb_close(MyLibusbDeviceHandle);
    }
    Connection_Status = 0;
}

struct libusb_transfer* atransfer;

int resubmit;
int totalBytes = 0;

int pollComplete() {
    //libusb_handle_events(NULL);
    struct timeval timeout = {0, 10000};
    //libusb_handle_events(NULL); fprintf(stderr, "*");
    libusb_handle_events_timeout(NULL, &timeout);
    return resubmit;
}

int cancelUSBTransfer() {
    libusb_cancel_transfer(atransfer);
    pollComplete();
}

void freeUSBTransfer() {
    libusb_free_transfer(atransfer);
}

static void kolbask(struct libusb_transfer* t) {
    int i;
    int32_t* bleh;

    if (t == NULL) {
        fprintf(stderr, "kolbask: vaya! null transfer\n");
    }

    if (t->status != 0) {
        resubmit = 2;

        fprintf(stderr, "kolbask: status=%d length=%d %x\n", 
                t->status, t->actual_length, t->buffer[0]);
    } else {
        resubmit = 1;
    }

    totalBytes += t->actual_length;
}

void setupTransfer(uint8_t* buffer, uint32_t size) {
    atransfer = libusb_alloc_transfer(0);   // 0 iso
    libusb_fill_bulk_transfer(atransfer, 
        MyLibusbDeviceHandle, 
        EP_IN, 
        buffer,
        size,
        kolbask,
        (void *) 0,
        1000);
    atransfer->flags = LIBUSB_TRANSFER_SHORT_NOT_OK;

    resubmit = 1;
}

int submitTransfer(uint8_t* buffer) {
    int retval;

    if (resubmit) {
        resubmit = 0;
        atransfer->buffer = buffer;
        retval = libusb_submit_transfer(atransfer);
        return (retval < 0) ? retval : 1;
    }
    return 0;
}

void sendUSBCommand(uint32_t command) {
    int bytes_sent;
    uint32_t outbuf[2];

    outbuf[0] = command;
    if(libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, (uint8_t*)outbuf, 
            8, &bytes_sent, 5000) != 0) {
        fprintf(stderr, "Failed to send request\n");
    }
}

#ifdef BENCHMARK
void benchmark() {
    struct timeval starttime, endtime, difftime;

    int i;

    gettimeofday(&starttime, NULL);

    printf("async gruuu..\n");

    setupTransfer();
    while(totalBytes < 49152*50) {
        submitTransfer();
        libusb_handle_events(NULL);
    }

    printf("\n");
    gettimeofday(&endtime, NULL);
    timersub(&endtime,&starttime,&difftime);
    printf("Received %d bytes in %d.%06d seconds\n",
        totalBytes,
        difftime.tv_sec,
        difftime.tv_usec);
    printf("Transfer speed: %f kbytes/sec\n",
        totalBytes/1024.0/(difftime.tv_sec+difftime.tv_usec/1e6));
}


main() {
    connectDevice();
    printf("Connected\n");
    
    benchmark();

    disconnectDevice();
    printf("Disconnected\n");
}

#endif
