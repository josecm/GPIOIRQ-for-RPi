#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "gpioirq.h"

int fd_pin22, fd_pin26;

void handler(int sig, siginfo_t *info, void* context){

	int tempfd;

	if(sig == SIGIO){
		
		tempfd = info->si_fd;

		if(tempfd == fd_pin22) {
			printf("Interrupt from pin 22!\n");
		} else if(tempfd == fd_pin26){
			printf("Interrupt from pin 26!\n");
		} else {
			printf("Interrupt from unknown pin!\n");
		}

	}

	return;
}

void closeuphandle(int sig){

	if(sig == SIGINT){

		printf("Bye!\n");
		close(fd_pin22);
		close(fd_pin26);

		_exit(0);
	}

}

int main(int argc, char **argv){
	
	int oflags;
	struct sigaction sig;
	memset(&sig, 0, sizeof(struct sigaction));

	signal(SIGINT, closeuphandle);

	/* opens device associated with interrupt on gpio 22 */
	if((fd_pin22 = open("/dev/gpio_irq22", O_RDWR)) < 0){
		perror("Failed to open /dev/gpio_irq22");
		return -1;
	}


	/* opens device associated with interrupt on gpio 26 */
	if((fd_pin26 = open("/dev/gpio_irq26", O_RDWR)) < 0){
		perror("Failed to open /dev/gpio_irq26");
		return -1;
	}

	/*	registers the handler for SIGIO and asks for siginfo_t struct to be used*/
	sig.sa_flags = SA_SIGINFO;
	sigemptyset(&(sig.sa_mask));
	sig.sa_sigaction = handler;	
	if(sigaction(SIGIO, &sig, NULL) < 0){
		perror("Failed to register handler with sigaction");
		return -1;
	}

	/* Register the process to receive SIGIO realtime signals related to fd_pin22 */
	fcntl(fd_pin22, F_SETSIG, SIGIO);
	fcntl(fd_pin22, F_SETOWN, getpid());
	oflags = fcntl(fd_pin22, F_GETFL);
	fcntl(fd_pin22, F_SETFL, oflags | O_ASYNC);

	/* Register the process to receive SIGIO realtime signals related to fd_pin26 */
	fcntl(fd_pin26, F_SETSIG, SIGIO);
	fcntl(fd_pin26, F_SETOWN, getpid());
	oflags = fcntl(fd_pin26, F_GETFL);
	fcntl(fd_pin26, F_SETFL, oflags | O_ASYNC);

	/* Configures the irq on gpio 26 to react to rising edge*/
	ioctl(fd_pin26, GPIOIRQ_IOC_SETTYPE, GPIOIRQ_RISING_EDGE);

	printf("Waiting for GPIO interrupts through SIGIO signals... CTRL-C to exit.\n");
	for(;;);

	return 0;
}