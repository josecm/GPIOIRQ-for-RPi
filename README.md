# GPIOIRQ-for-RPi

Linux kernel module for Raspberry Pi enabling the use of GPIO as external interrupt sources.
The driver registers a device for each gpio present on the raspberry pi header. The processes with open devices and configured file descriptors will receive a realtime SIGIO signal when an interrupt on any of gpios associated with the open devices is generated. A process with several opened devices can distiguish them in the signal handler by reading the si_fd field in the siginfo_t parameter and checking wich of the opened devices with that file pin generated the interrupt.


/* Building and loading the module */

You'll need to set the environment variable CROSS to your cross-compile toolchain prefix and the KDIR variable to where your kernel source is located. The Makefile has this variables set by default to my toolchain and kernel source directory under buildroot.

Once the module .ko file is on your target machine, load it using the insmod command line tool.

After loading the module the /dev/ directory should have some gpioirq_xx character devices, 
each one representing one pin present on the raspberry pi header.


/* How to use it */

The devices created can only be opened by one process at a time.

In your application you will first need to insert the <fcntl.h> and <signal.h> headers.


An example of how to use this devices is provided in the test_module.c program.
