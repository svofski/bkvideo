#define BENCHMARK
#define BENCHTIME   5
#define BAUDRATE 1500000
#define BENCHSIZE   5*1024*1024

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <AvailabilityMacros.h>

#ifdef __MWERKS__
#define __CF_USE_FRAMEWORK_INCLUDES__
#endif

#include <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
#include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

// Hold the original termios attributes so we can reset them
static struct termios gOriginalTTYAttrs;

// Function prototypes
static int OpenSerialPort(const char *bsdPath);
static char *LogString(char *str);
static Boolean InitializeModem(int fileDescriptor);
static void CloseSerialPort(int fileDescriptor);

// Given the path to a serial device, open the device and configure it.
// Return the file descriptor associated with the device.
static int OpenSerialPort(const char *bsdPath)
{
    int				fileDescriptor = -1;
    int				handshake;
    struct termios	options;
    
    // Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
    // The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
    // See open(2) ("man 2 open") for details.
    
    fileDescriptor = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fileDescriptor == -1)
    {
        printf("Error opening serial port %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }

    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
    if (ioctl(fileDescriptor, TIOCEXCL) == -1)
    {
        printf("Error setting TIOCEXCL on %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
    // See fcntl(2) ("man 2 fcntl") for details.
    
    if (fcntl(fileDescriptor, F_SETFL, 0) == -1)
    {
        printf("Error clearing O_NONBLOCK %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Get the current options and save them so we can restore the default settings later.
    if (tcgetattr(fileDescriptor, &gOriginalTTYAttrs) == -1)
    {
        printf("Error getting tty attributes %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
        goto error;
    }

    // The serial port attributes such as timeouts and baud rate are set by modifying the termios
    // structure and then calling tcsetattr() to cause the changes to take effect. Note that the
    // changes will not become effective without the tcsetattr() call.
    // See tcsetattr(4) ("man 4 tcsetattr") for details.
    
    options = gOriginalTTYAttrs;
    
    // Set raw input (non-canonical) mode, with reads blocking until either a single character 
    // has been received or a one second timeout expires.
    // See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.
    
    cfmakeraw(&options);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;
        
#if 1
    // The baud rate, word length, and handshake options can be set as follows:
    options.c_cflag |= (CS8 	   | 	// Use 8 bit words
    					CCTS_OFLOW | 	// CTS flow control of output
    					CRTS_IFLOW);	// RTS flow control of input
#endif			

#if 1
    // Cause the new options to take effect immediately.
    if (tcsetattr(fileDescriptor, TCSANOW, &options) == -1)
    {
        printf("Error setting tty attributes %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
        goto error;
    }
#endif

#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
	// Starting with Tiger, the IOSSIOSPEED ioctl can be used to set arbitrary baud rates
	// other than those specified by POSIX. The driver for the underlying serial hardware
	// ultimately determines which baud rates can be used. This ioctl sets both the input
	// and output speed. 
	
    // 1.25 mbps
	speed_t speed = BAUDRATE;
    if (ioctl(fileDescriptor, IOSSIOSPEED, &speed) == -1)
    {
        printf("Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
    }
#endif
    
    // To set the modem handshake lines, use the following ioctls.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    if (ioctl(fileDescriptor, TIOCSDTR) == -1) // Assert Data Terminal Ready (DTR)
    {
        printf("Error asserting DTR %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
    }
    
    if (ioctl(fileDescriptor, TIOCCDTR) == -1) // Clear Data Terminal Ready (DTR)
    {
        printf("Error clearing DTR %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
    }
    
    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
    if (ioctl(fileDescriptor, TIOCMSET, &handshake) == -1)
    // Set the modem lines depending on the bits set in handshake
    {
        printf("Error setting handshake lines %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
    }
    
    // To read the state of the modem lines, use the following ioctl.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.
    
    if (ioctl(fileDescriptor, TIOCMGET, &handshake) == -1)
    // Store the state of the modem lines in handshake
    {
        printf("Error getting handshake lines %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
    }
    
    printf("Handshake lines currently set to %d\n", handshake);
	
#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
	unsigned long mics = 1UL;

	// Set the receive latency in microseconds. Serial drivers use this value to determine how often to
	// dequeue characters received by the hardware. Most applications don't need to set this value: if an
	// app reads lines of characters, the app can't do anything until the line termination character has been
	// received anyway. The most common applications which are sensitive to read latency are MIDI and IrDA
	// applications.
	
	if (ioctl(fileDescriptor, IOSSDATALAT, &mics) == -1)
	{
		// set latency to 1 microsecond
        printf("Error setting read latency %s - %s(%d).\n",
            bsdPath, strerror(errno), errno);
        goto error;
	}
#endif
    
    // Success
    return fileDescriptor;
    
    // Failure path
error:
    if (fileDescriptor != -1)
    {
        close(fileDescriptor);
    }
    
    return -1;
}

// Replace non-printable characters in str with '\'-escaped equivalents.
// This function is used for convenient logging of data traffic.
static char *LogString(char *str)
{
    static char     buf[2048];
    char            *ptr = buf;
    int             i;

    *ptr = '\0';

    while (*str)
	{
		if (isprint(*str))
		{
			*ptr++ = *str++;
		}
		else {
			switch(*str)
			{
				case ' ':
					*ptr++ = *str;
					break;

				case 27:
					*ptr++ = '\\';
					*ptr++ = 'e';
					break;

				case '\t':
					*ptr++ = '\\';
					*ptr++ = 't';
					break;

				case '\n':
					*ptr++ = '\\';
					*ptr++ = 'n';
					break;

				case '\r':
					*ptr++ = '\\';
					*ptr++ = 'r';
					break;

				default:
					i = *str;
					(void)sprintf(ptr, "\\%03o", i);
					ptr += 4;
					break;
			}

			str++;
		}

		*ptr = '\0';
	}

    return buf;
}

static volatile int totalBytesRead = 0;

void alarmhandler(int s) {
    printf("Alarm\n");
    printf("Total bytes read in %s seconds = %d (%d bytes/sec)\n",  
        BENCHTIME, 
        totalBytesRead,
        totalBytesRead/BENCHTIME);

    exit(0);
}


// Given the file descriptor for a modem device, attempt to initialize the modem by sending it
// a standard AT command and reading the response. If successful, the modem's response will be "OK".
// Return true if successful, otherwise false.
static Boolean ReceiveData(int fileDescriptor)
{
    char		buffer[32768];	// Input buffer
    char		*bufPtr;		// Current char in buffer
    ssize_t		numBytes;		// Number of bytes read or written
    Boolean		result = false;

    struct timeval starttime, endtime, difftime;

    totalBytesRead = 0;
    
    bufPtr = buffer;
#ifdef BENCHMARK
    printf("Waiting for start...");
    read(fileDescriptor, bufPtr, 2); // wait for start
    printf("OK\n");

    gettimeofday(&starttime, NULL);

    //signal(SIGALRM, alarmhandler);
    //alarm(BENCHTIME);
#endif

    do {
#ifndef BENCHMARK
        signal(SIGALRM, alarmhandler);
        alarm(5);
#endif
        numBytes = read(fileDescriptor, bufPtr, sizeof(buffer) - 1);
#ifndef BENCHMARK
        signal(SIGALRM, NULL);
#endif
#ifdef BENCHMARK
        totalBytesRead += numBytes;
#else
        if (numBytes > 0) {
            bufPtr[numBytes] = '\0';
            printf("%s", buffer);
        } 
#endif
        if (totalBytesRead >= BENCHSIZE) {
            gettimeofday(&endtime, NULL);
            timersub(&endtime,&starttime,&difftime);
            printf("Received %d bytes in %d.%d seconds\n", 
                totalBytesRead,
                difftime.tv_sec, 
                difftime.tv_usec);
            printf("Transfer speed: %f kbytes/sec\n", 
                totalBytesRead/1024.0/(difftime.tv_sec+difftime.tv_usec/1e6));
                
            break;
        }
    } while (numBytes > 0);

    return result;
}

// Given the file descriptor for a serial device, close that device.
void CloseSerialPort(int fileDescriptor)
{
    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver. 
	// See tcsendbreak(3) ("man 3 tcsendbreak") for details.
    if (tcdrain(fileDescriptor) == -1)
    {
        printf("Error waiting for drain - %s(%d).\n",
            strerror(errno), errno);
    }
    
    // Traditionally it is good practice to reset a serial port back to
    // the state in which you found it. This is why the original termios struct
    // was saved.
    if (tcsetattr(fileDescriptor, TCSANOW, &gOriginalTTYAttrs) == -1)
    {
        printf("Error resetting tty attributes - %s(%d).\n",
            strerror(errno), errno);
    }

    close(fileDescriptor);
}

int main(int argc, char **argv)
{
    int			fileDescriptor;
    kern_return_t	kernResult; // on PowerPC this is an int (4 bytes)
/*
 *	error number layout as follows (see mach/error.h):
 *
 *	hi		 		       lo
 *	| system(6) | subsystem(12) | code(14) |
 */

    char		bsdPath[MAXPATHLEN];


    if (argc > 1) {
        strncpy(bsdPath, argv[1], MAXPATHLEN);
    } else {
        strcpy(bsdPath, "/dev/tty.usbserial-A5004H5Q");
    }   
 
    // Now open the modem port we found, initialize the modem, then close it
    if (!bsdPath[0])
    {
        printf("No modem port found.\n");
        return EX_UNAVAILABLE;
    }

    fileDescriptor = OpenSerialPort(bsdPath);
    if (-1 == fileDescriptor)
    {
        return EX_IOERR;
    }

    ReceiveData(fileDescriptor);
        
    CloseSerialPort(fileDescriptor);

    printf("\n");
        
    return EX_OK;
}
