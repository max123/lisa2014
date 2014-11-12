/*
 * the crash driver.  Modified from scull driver by Rubini and Corbet
 * (see below).
 */

/*
 * main.c -- the bare crash char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#ifdef NOTNOW
#include <asm/system.h>		/* cli(), *_flags */
#endif
#include <asm/uaccess.h>	/* copy_*_user */

#include "crash.h"		/* local definitions */

/*
 * Our parameters which can be set at load time.
 */

int crash_major =   CRASH_MAJOR;
int crash_minor =   0;
int crash_nr_devs = CRASH_NR_DEVS;	/* number of bare crash devices */
int crash_quantum = CRASH_QUANTUM;
int crash_qset =    CRASH_QSET;

module_param(crash_major, int, S_IRUGO);
module_param(crash_minor, int, S_IRUGO);
module_param(crash_nr_devs, int, S_IRUGO);
module_param(crash_quantum, int, S_IRUGO);
module_param(crash_qset, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet, Max Bruning");
MODULE_LICENSE("Dual BSD/GPL");

struct crash_dev *crash_devices;	/* allocated in crash_init_module */


/*
 * Empty out the crash device; must be called with the device
 * semaphore held.
 */
int crash_trim(struct crash_dev *dev)
{
	struct crash_qset *next, *dptr;
	int qset = dev->qset;   /* "dev" is not-null */
	int i;

	for (dptr = dev->data; dptr; dptr = next) { /* all the list items */
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = crash_quantum;
	dev->qset = crash_qset;
	dev->data = NULL;
	return 0;
}

/*
 * Open and close
 */

int crash_open(struct inode *inode, struct file *filp)
{
	struct crash_dev *dev; /* device information */

	dev = container_of(inode->i_cdev, struct crash_dev, cdev);
	filp->private_data = dev; /* for other methods */

	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		crash_trim(dev); /* ignore errors */
		up(&dev->sem);
	}
	return 0;          /* success */
}

int crash_release(struct inode *inode, struct file *filp)
{
	return 0;
}
/*
 * Follow the list
 */
struct crash_qset *crash_follow(struct crash_dev *dev, int n)
{
	struct crash_qset *qs = dev->data;

        /* Allocate first qset explicitly if need be */
	if (! qs) {
		qs = dev->data = kmalloc(sizeof(struct crash_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct crash_qset));
	}

	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct crash_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct crash_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

/*
 * The ioctl() implementation
 */

caddr_t crash_addr;

long crash_ioctl(struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int retval = 0;
    
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != CRASH_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > CRASH_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {

	case CRASH_ALLOC:
	{
		crash_addr = kmalloc(4096, GFP_KERNEL);
		kfree(crash_addr);
		break;
	}
        case CRASH_USEAFTERFREE:
		memset(crash_addr, '?', 4096);
		break;

	case CRASH_NULLPOINTER:
		*crash_addr = (char *)'?';
		break;
	case CRASH_STACKOVERFLOW:
	{
		volatile char toobig[10000];
		volatile int i;
		for (i = 0; i < sizeof(toobig); i++)
			toobig[i] = '?';
		break;
	}
	default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;

}


struct file_operations crash_fops = {
	.owner =    THIS_MODULE,
	.unlocked_ioctl =    crash_ioctl,
	.open =     crash_open,
	.release =  crash_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void crash_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(crash_major, crash_minor);

	/* Get rid of our char dev entries */
	if (crash_devices) {
		for (i = 0; i < crash_nr_devs; i++) {
			crash_trim(crash_devices + i);
			cdev_del(&crash_devices[i].cdev);
		}
		kfree(crash_devices);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, crash_nr_devs);

}


/*
 * Set up the char_dev structure for this device.
 */
static void crash_setup_cdev(struct crash_dev *dev, int index)
{
	int err, devno = MKDEV(crash_major, crash_minor + index);
    
	cdev_init(&dev->cdev, &crash_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &crash_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding crash%d", err, index);
}


int crash_init_module(void)
{
	int result, i;
	dev_t dev = 0;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (crash_major) {
		dev = MKDEV(crash_major, crash_minor);
		result = register_chrdev_region(dev, crash_nr_devs, "crash");
	} else {
		result = alloc_chrdev_region(&dev, crash_minor, crash_nr_devs,
				"crash");
		crash_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "crash: can't get major %d\n", crash_major);
		return result;
	}

        /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	crash_devices = kmalloc(crash_nr_devs * sizeof(struct crash_dev), GFP_KERNEL);
	if (!crash_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(crash_devices, 0, crash_nr_devs * sizeof(struct crash_dev));

        /* Initialize each device. */
	for (i = 0; i < crash_nr_devs; i++) {
		crash_devices[i].quantum = crash_quantum;
		crash_devices[i].qset = crash_qset;
		sema_init(&crash_devices[i].sem,1);
		crash_setup_cdev(&crash_devices[i], i);
	}

        /* At this point call the init function for any friend device */
	dev = MKDEV(crash_major, crash_minor + crash_nr_devs);

	return 0; /* succeed */

  fail:
	crash_cleanup_module();
	return result;
}

module_init(crash_init_module);
module_exit(crash_cleanup_module);
