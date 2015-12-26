

ifneq ($(KERNELRELEASE),)

	obj-m := gpioirq.o

else

	KDIR = $(HOME)/buildroot/buildroot-2015.08.1/output/build/linux-rpi-4.1.y
	PWD = $(shell pwd)
	CROSS = $(HOME)/buildroot/buildroot-2015.08.1/output/host/usr/bin/arm-buildroot-linux-uclibcgnueabihf-

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(CROSS) modules

clean:
	rm -rf *.o *.ko *.mod.* *.order *.symvers

endif
