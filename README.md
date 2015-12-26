# GPIOIRQ-for-RPi

Linux kernel module for Raspberry Pi enabling the use of GPIO as external interrupt sources.
The driver registers a device for each gpio present on the raspberry pi header. The processes with open devices and configured file descriptors will receive a realtime SIGIO signal when an interrupt on any of gpios associated with the open devices is generated. A process with several opened devices can distiguish the source of the interrupt by reading the si_fd field in the siginfo_t parameter of the signal handlerand checking which of the opened devices with that file descriptor generated the signal and consequently the interrupt.
It is also possible to control the type of event that generates the interrupt (falling edge - default, rising edge, high level or low level) and configure the pins associated pull-up/down structure, through the use of ioctl commands.


/* Building and loading the module */

You'll need to set the environment variable CROSS to your cross-compile toolchain prefix and the KDIR variable to where your kernel source is located and run make. The Makefile has this variables set by default to my toolchain and kernel source directory under buildroot.

Once the module .ko file is on your target machine, load it using the insmod command line tool.

After loading the module the /dev/ directory should have some gpioirq_xx character devices, 
each one representing one gpio present on the raspberry pi header.


/* How to use it */


In your application you will first need to insert the <fcntl.h> and <signal.h> and "gpioirq.h" headers.

You will need to follow these steps to configure your program to receive interrupts from the pins:

1) Open the device(s) associated with the gpios for which you want to receive the signal/interrupt.
2) 

The devices created can only be opened by one process at a time.

An example of how to use this devices is provided in the test_module.c program.
