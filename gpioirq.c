
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/atomic.h>
#include <linux/gpio.h>
#include <linux/interrupt.h> 
#include <mach/platform.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "gpioirqdev.h"

#define CLASS_NAME "GPIO_IRQ"

#define N_GPIO_HEADER	26

#define GPFSEL0 	(0x00)
#define GPFSEL1 	(0x04)
#define GPFSEL2 	(0x08)
#define GPFSEL3 	(0x0C)
#define GPFSEL4 	(0x10)
#define GPFSEL5		(0x14)

#define GPREN0 		(0X4C)
#define GPFEN0		(0X58)
#define GPHEN0		(0X64)
#define GPLEN0 		(0X70)

#define IRQEN2		(0x14)
#define IRQDS2 		(0x20)

#define GPPUD 		(0x94)
#define GPPUDCLK0	(0x98)

MODULE_AUTHOR("Jose Martins - josemartins90@gmail.com");
MODULE_LICENSE("GPL");

static const int 
gpio_header_pins[N_GPIO_HEADER] = {2, 3, 4, 17, 27, 22, 10, 9, 11, 5, 6, 13, 19, 26, 14, 15, 18, 23, 24, 25, 8,7 ,12, 16, 20, 21};

struct gpio_irq {

	int pin;
	int irq;
	struct cdev c_dev;
	struct fasync_struct *fa;
	dev_t majorminor;
	atomic_t atom;
	unsigned type;
	unsigned pull;

};

static struct gpio_irq *gpio_irq_list;

static struct class *gpioirq_class = NULL;
static dev_t majorminor;

static void *gpio_base, *irqctrl_base;

irqreturn_t myhandler(int irq, void *dev_id){

	struct gpio_irq *gpir = dev_id;

	printk(KERN_INFO "GPIO interrupt generated by pin %d\n", gpir->pin);

	if(gpir->fa){
		kill_fasync(&(gpir->fa), SIGIO, POLL_IN);
	}

	return IRQ_HANDLED;
}


/* THIS WAS NOT TESTED */
static int gpioirq_setpull(struct gpio_irq *gpir, int type){

	if(type == GPIOIRQ_PULLUP){
		iowrite32(0x02, gpio_base + GPPUD);
	} else if (type == GPIOIRQ_PULLUP){
		iowrite32(0x01, gpio_base + GPPUD);
	} else if (type == GPIOIRQ_PULLUP) {
		iowrite32(0x00, gpio_base + GPPUD);
	} else {
		return -EINVAL;
	}

	udelay(1);
	iowrite32(1 << gpir->pin, gpio_base + GPPUDCLK0);
	udelay(1);
	iowrite32(0,  gpio_base + GPPUDCLK0);

	return 0;
}

static int gpioirq_settype(struct gpio_irq *gpir, unsigned long type){

	unsigned long temp = 0;

	printk("ioctl arg = 0x%x", type);
	printk("gpir type = 0x%x", gpir->type);


	if((type & GPIOIRQ_FALLING_EDGE) && !(gpir->type & type)){
		printk(KERN_INFO "1\n");
		temp = ioread32(gpio_base + GPFEN0);
		temp |= (1 << gpir->pin);
		iowrite32(temp, gpio_base + GPFEN0);
		gpir->type |= GPIOIRQ_FALLING_EDGE;
	} else if(!(type & GPIOIRQ_FALLING_EDGE) && (gpir->type & GPIOIRQ_FALLING_EDGE)){
		printk(KERN_INFO "2\n");
		temp = ioread32(gpio_base + GPFEN0);
		printk(KERN_INFO "temp: 0x%x\n", temp);
		temp &= ~(1 << gpir->pin);
		printk(KERN_INFO "temp &= ~(1 << gpir->pin): 0x%x\n", temp);
		iowrite32(temp, gpio_base + GPFEN0);
		gpir->type &= ~GPIOIRQ_FALLING_EDGE;
	}

	if(type & GPIOIRQ_RISING_EDGE && !(gpir->type & type)){
		printk(KERN_INFO "3\n");
		temp = ioread32(gpio_base + GPREN0);
		temp |= (1 << gpir->pin);
		iowrite32(temp, gpio_base + GPREN0);
		gpir->type |= GPIOIRQ_RISING_EDGE;
	} else if(!(type & GPIOIRQ_RISING_EDGE) && (gpir->type & GPIOIRQ_RISING_EDGE)){
		printk(KERN_INFO "4\n");
		temp = ioread32(gpio_base + GPREN0);
		temp &= ~(1 << gpir->pin);
		iowrite32(temp, gpio_base + GPREN0);
		gpir->type &= ~GPIOIRQ_RISING_EDGE;
	}

	if(type & GPIOIRQ_HIGH && !(gpir->type & type)){
		printk(KERN_INFO "5\n");
		temp = ioread32(gpio_base + GPHEN0);
		temp |= (1 << gpir->pin);
		iowrite32(temp, gpio_base + GPHEN0);
		gpir->type |= GPIOIRQ_HIGH;
	} else if(!(type & GPIOIRQ_HIGH) && (gpir->type & GPIOIRQ_HIGH)){
		printk(KERN_INFO "6\n");
		temp = ioread32(gpio_base + GPHEN0);
		temp &= ~(1 << gpir->pin);
		iowrite32(temp, gpio_base + GPHEN0);
		gpir->type &= ~GPIOIRQ_HIGH;
	}

	if(type & GPIOIRQ_LOW && !(gpir->type & type)){
		printk(KERN_INFO "7\n");
		temp = ioread32(gpio_base + GPLEN0);
		temp |= (1 << gpir->pin);
		iowrite32(temp, gpio_base + GPLEN0);
		gpir->type |= GPIOIRQ_LOW;

	} else if(!(type & GPIOIRQ_LOW) && (gpir->type & GPIOIRQ_LOW)){
		printk(KERN_INFO "8\n");
		temp = ioread32(gpio_base + GPLEN0);
		temp &= ~(1 << gpir->pin);
		iowrite32(temp, gpio_base + GPLEN0);
		gpir->type &=  ~GPIOIRQ_LOW;
	}


	return 0;
}

static int my_open(struct inode *node, struct file *file){

	unsigned long temp = 0;
	int offset;
	struct gpio_irq *gpir = container_of(node->i_cdev, struct gpio_irq, c_dev);

	if(!atomic_dec_and_test(&(gpir->atom))){
		atomic_inc(&(gpir->atom));
		return -EBUSY;
	}

	file->private_data = gpir;
	gpir->irq = gpio_to_irq(gpir->pin);

	if(request_irq(gpir->irq, myhandler, 0, "GPIO_IRQ", gpir)){
		printk(KERN_INFO "Failed Requesting Interrupt Line...");
		return -1;
	}

	if(gpir->pin >= 0 && gpir->pin <= 9){
		offset = GPFSEL0;
	} else if (gpir->pin >= 10 && gpir->pin <= 19){
		offset = GPFSEL1;
	} else {
		offset = GPFSEL2;
	}

	/* Configure pin as input */
	temp = ioread32(gpio_base + offset);
	temp &= (7 << ((gpir->pin % 10)) * 3);
	iowrite32(temp, gpio_base  + offset);

	/* Interrupt type is set falling edge by default */
	gpir->type = GPIOIRQ_NONE;
	gpioirq_settype(gpir, GPIOIRQ_FALLING_EDGE);
	
	return 0;
}

static int my_release(struct inode *node, struct file *file){

	struct gpio_irq *gpir = file->private_data;

	//Disbales Interrupt for pin
	gpioirq_settype(gpir, GPIOIRQ_NONE);

	fasync_helper(-1, file, 0, &(gpir->fa));
	free_irq(gpir->irq, gpir);
	atomic_inc(&(gpir->atom));

	return 0;
}

static int my_fasync(int fd, struct file *filp, int mode){

	int ret;
	struct gpio_irq *gpir = filp->private_data;

	if((ret = fasync_helper(fd, filp, mode, &(gpir->fa))) < 0){
		printk(KERN_INFO "Fasync Failed: %d\n", ret);
		return ret;
	}

	return 0;
}



static long my_ioctl(struct file *fp, unsigned int cmd, unsigned long arg){

	struct gpio_irq *gpir = fp->private_data;

	switch(cmd){

		case GPIOIRQ_IOC_SETTYPE :
			return gpioirq_settype(gpir, arg);
			break;
		case GPIOIRQ_IOC_SETPULL:
			return gpioirq_setpull(gpir, arg);
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

static struct file_operations fops = {

	.owner = THIS_MODULE,
	.open = my_open,
	.fasync = my_fasync,
	.release = my_release,
	.unlocked_ioctl = my_ioctl

};


static __init int my_init(void){

	unsigned long temp = 0;
	int i, j,  ret = 0;
	dev_t tempmajorminor;
	char device_name[11];

	if((ret = alloc_chrdev_region(&majorminor, 0, N_GPIO_HEADER, CLASS_NAME)) < 0){
		printk(KERN_INFO "Failed allocating major...\n");
		return ret;
	}

	if(IS_ERR(gpioirq_class = class_create(THIS_MODULE, CLASS_NAME))){
		printk(KERN_INFO "Failed creating class..\n");
		ret = PTR_ERR(gpioirq_class);
		goto classcreat_err;
	}

	gpio_irq_list = kmalloc(N_GPIO_HEADER * sizeof(struct gpio_irq), GFP_KERNEL);
	if(!gpio_irq_list){
		ret = ENOMEM;
		goto classcreat_err;
	}
	memset(gpio_irq_list, 0, N_GPIO_HEADER * sizeof(struct gpio_irq));


	for(i = 0; i < N_GPIO_HEADER; i++){

		gpio_irq_list[i].pin = gpio_header_pins[i];
		gpio_irq_list[i].fa = NULL;
		
		snprintf(device_name, 11, "gpio_irq%d", gpio_header_pins[i]);

		tempmajorminor = MKDEV(MAJOR(majorminor), i );
		gpio_irq_list[i].majorminor = tempmajorminor;

		if(IS_ERR(ret = device_create(gpioirq_class, NULL, tempmajorminor, NULL, device_name))){
			printk(KERN_INFO "Failed creating device...\n");
			ret = PTR_ERR(gpioirq_class);
			goto devicecreat_err;
		}

		cdev_init(&(gpio_irq_list[i].c_dev), &fops);
		gpio_irq_list[i].c_dev.owner = THIS_MODULE;
		gpio_irq_list[i].c_dev.ops = &fops;

		if((ret = (cdev_add(&(gpio_irq_list[i].c_dev), tempmajorminor, 1))) < 0){
			printk(KERN_INFO "Failed register device...\n");
			goto devadd_err;
		}

		atomic_set(&(gpio_irq_list[i].atom), 1);

	}

	if((gpio_base = ioremap(GPIO_BASE, 0x60)) == NULL){
		printk(KERN_INFO "Failed mapping gpio registers...\n");
		ret = -1;
		goto devicecreat_err;
	}
	if((irqctrl_base = ioremap(ARMCTRL_IC_BASE, 0x28)) == NULL){
		printk(KERN_INFO "Failed mapping interrupt control registers...\n");
		ret = -1;
	}

	/* Enables interrupts for gpio 0-31*/
	temp = ioread32(irqctrl_base + IRQEN2);
	temp = (1 << 17);
	iowrite32(temp, irqctrl_base + IRQEN2);


	printk(KERN_INFO "GPIO_IRQ Module initialized.\n");
	return 0;

ioremap_err:
	iounmap(gpio_base);
devadd_err:	
	device_destroy(gpioirq_class, gpio_irq_list[i].majorminor);
devicecreat_err:
	for(j = 0; j < i; j++){
		cdev_del(&(gpio_irq_list[j].c_dev));
		device_destroy(gpioirq_class, gpio_irq_list[j].majorminor);
	}
	class_destroy(gpioirq_class);
classcreat_err:
	unregister_chrdev_region(majorminor, 1);
	return ret;
}

static __exit void my_exit(void){

	int i;
	unsigned long temp = 0;

	for(i = 0; i < N_GPIO_HEADER; i++){
		cdev_del(&(gpio_irq_list[i].c_dev));
		device_destroy(gpioirq_class, gpio_irq_list[i].majorminor);
	}

	class_destroy(gpioirq_class);
	unregister_chrdev_region(majorminor, 1);

	/* Disables interrupts for gpio 0-31*/
	temp = ioread32(irqctrl_base + IRQDS2);
	temp = (1 << 17);
	iowrite32(temp, irqctrl_base + IRQDS2);

	iounmap(gpio_base);
	iounmap(irqctrl_base);


	printk(KERN_INFO "GPIO_IRQ Module exits...\n");

}

module_init(my_init);
module_exit(my_exit);