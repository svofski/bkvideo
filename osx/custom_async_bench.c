#define BENCHMARK

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
}

struct libusb_transfer* atransfer;

int resubmit;
int totalBytes = 0;

int pollComplete() {
    return resubmit;
}

static void kolbask(struct libusb_transfer* t) {
    int i;
    int32_t* bleh;

    fprintf(stderr, "kolbask: status=%d length=%d\n%s\n--- %d\n", 
        t->status, t->actual_length, t->buffer, strlen((char *)t->buffer));
    resubmit = 1;
    totalBytes += t->actual_length;

    bleh = (int32_t*)t->buffer;
    for (i = 0; i < t->actual_length/sizeof(int32_t); i++) {
        if (i == bleh[i]) continue;
        fprintf(stderr, "%04d:%04d  ", i, bleh[i]);
    }
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
        5000);

    resubmit = 1;
}

void submitTransfer() {
    int bytes_sent;
    uint8_t outbuf[EP_SIZE];

    if (resubmit) {
        outbuf[0] = 0x81;
        if(libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, outbuf, 
                8 /*EP_SIZE*/, &bytes_sent, 5000) != 0) {
            fprintf(stderr, "Failed to send request\n");
        }

        resubmit = 0;
        //printf("sub %d: %d\n", i, libusb_submit_transfer(atransfer));
        libusb_submit_transfer(atransfer);
    }
}

#ifdef BENCHMARK
void benchmark() {
    struct timeval starttime, endtime, difftime;
    uint8_t buffer[16000];

    int i;

    gettimeofday(&starttime, NULL);

    printf("async gruuu..\n");

    setupTransfer(buffer, sizeof(buffer));
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
