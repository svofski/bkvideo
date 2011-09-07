// Includes
#include <stdio.h>
#include <usb.h>
#include <time.h>
#include <sys/time.h>

//Modify this value to match the VID and PID in your USB device descriptor.//Modify this value to match the VID and PID in your USB device descriptor.
#define MY_VID 0x04D8
#define MY_PID 0x0204

/* the device's endpoints */
#define EP_IN 0x81
#define EP_OUT 0x01

#define CONNECTED  0x01

#define EP_SIZE 255

int Connection_Status = 0;

// Global Variables
usb_dev_handle *MyLibusbDeviceHandle = NULL;  /* the device handle */

/* The Below Function Searches for a device with given VID and PID.
It return a hanlde to the device if the device is found in the Bus.*/
usb_dev_handle *open_dev(void)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for(bus = usb_get_busses(); bus; bus = bus->next)
    {
    for(dev = bus->devices; dev; dev = dev->next)
    {
        if(dev->descriptor.idVendor == MY_VID
           && dev->descriptor.idProduct == MY_PID)
        {
        return usb_open(dev);  //Opens a USB device
        }
    }
    }
    return NULL;
}

void connectDevice() {
    if ( Connection_Status != CONNECTED)  // if not connected already
    {
    usb_init(); /* initialize the library */
    usb_find_busses(); /* find all busses */
    usb_find_devices(); /* find all connected devices */
    if(!(MyLibusbDeviceHandle = open_dev()))
    {
        fprintf(stderr, "open_dev() failed\n");
        return;
    }
    if(usb_set_configuration(MyLibusbDeviceHandle, 1) < 0) // Sets the Active configuration of the device
    {
        fprintf(stderr, "usb_set_configuration() failed\n");
        usb_close(MyLibusbDeviceHandle);
        return;
    }
    if(usb_claim_interface(MyLibusbDeviceHandle, 0) < 0)  //claims the interface with the Operating System
    {
        fprintf(stderr, "usb_claim_interface() failed\n");
        //Closes a device opened since the claim interface is failed.
        usb_close(MyLibusbDeviceHandle);
        return ;
    }
    Connection_Status = CONNECTED;     // Everything went well. Now connection status is CONNECTED
    }
}

void disconnectDevice() {
    if ( Connection_Status == CONNECTED) {
        //The following functiom releases a previously claimed interface
        usb_release_interface(MyLibusbDeviceHandle, 0);
        //closes a device opened
        usb_close(MyLibusbDeviceHandle);
    }
}

void queryData() {
    if ( Connection_Status  == CONNECTED) {
        char OutputPacketBuffer[EP_SIZE]; 
        char InputPacketBuffer[2][EP_SIZE]; 
        OutputPacketBuffer[0] = 0x81; //0x81 is the "Get Pushbutton State" command in the firmware

        /*
        //The following call to usb_bulk_write() sends EP_SIZE bytes of data to the USB device.
        if(usb_bulk_write(MyLibusbDeviceHandle, EP_OUT, &OutputPacketBuffer[0], EP_SIZE, 5000) != EP_SIZE) {
            fprintf(stderr, "queryData write failed\n");
            return;
        }
        */

        //Now get the response packet from the firmware.
        //The following call to usb_bulk_read() retrieves EP_SIZE bytes of data from the USB device.
        if(usb_bulk_read(MyLibusbDeviceHandle, EP_IN + 0, InputPacketBuffer[0], EP_SIZE, 5000) != EP_SIZE) {
            fprintf(stderr, "queryData read failed\n");
            return;
        }
        if(usb_bulk_read(MyLibusbDeviceHandle, EP_IN + 1, InputPacketBuffer[1], EP_SIZE, 5000) != EP_SIZE) {
            fprintf(stderr, "queryData read failed\n");
            return;
        }

        //printf("Received: [%s]", InputPacketBuffer);
    }
}

void benchmark() {
    struct timeval starttime, endtime, difftime;

    int i;

    gettimeofday(&starttime, NULL);

    printf("gruuu..\n");
    // receiving 5MB of data
    for (i = 0; i < 16384; i++) {
        if (i % 100 == 0) printf("measuring %08d\r");
        queryData();
    }
    printf("\n");
    gettimeofday(&endtime, NULL);
    timersub(&endtime,&starttime,&difftime);
    printf("Received %d bytes in %d.%d seconds\n",
        16384*1*EP_SIZE,
        difftime.tv_sec,
        difftime.tv_usec);
    printf("Transfer speed: %f kbytes/sec\n",
        1*16384*EP_SIZE/1024.0/(difftime.tv_sec+difftime.tv_usec/1e6));
}

main() {
    connectDevice();
    printf("Connected\n");
//    queryData();
    benchmark();
    disconnectDevice();
    printf("Disconnected\n");
}
