/*
 * crash.h -- definitions for the char module
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
 * $Id: crash.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */

#ifndef _CRASH_H_
#define _CRASH_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef CRASH_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "crash: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#ifndef CRASH_MAJOR
#define CRASH_MAJOR 0   /* dynamic major by default */
#endif

#ifndef CRASH_NR_DEVS
#define CRASH_NR_DEVS 4    /* crash0 through crash3 */
#endif

#ifndef CRASH_P_NR_DEVS
#define CRASH_P_NR_DEVS 4  /* crashpipe0 through crashpipe3 */
#endif

/*
 * The bare device is a variable-length region of memory.
 * Use a linked list of indirect blocks.
 *
 * "crash_dev->data" points to an array of pointers, each
 * pointer refers to a memory area of CRASH_QUANTUM bytes.
 *
 * The array (quantum-set) is CRASH_QSET long.
 */
#ifndef CRASH_QUANTUM
#define CRASH_QUANTUM 4000
#endif

#ifndef CRASH_QSET
#define CRASH_QSET    1000
#endif

/*
 * The pipe device is a simple circular buffer. Here its default size
 */
#ifndef CRASH_P_BUFFER
#define CRASH_P_BUFFER 4000
#endif

/*
 * Representation of crash quantum sets.
 */
struct crash_qset {
	void **data;
	struct crash_qset *next;
};

#ifdef __KERNEL__

struct crash_dev {
	struct crash_qset *data;  /* Pointer to first quantum set */
	int quantum;              /* the current quantum size */
	int qset;                 /* the current array size */
	unsigned long size;       /* amount of data stored here */
	unsigned int access_key;  /* used by crashuid and crashpriv */
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
};
#endif /*__KERNEL__*/
/*
 * Split minors in two parts
 */
#define TYPE(minor)	(((minor) >> 4) & 0xf)	/* high nibble */
#define NUM(minor)	((minor) & 0xf)		/* low  nibble */


/*
 * The different configurable parameters
 */
extern int crash_major;     /* main.c */
extern int crash_nr_devs;
extern int crash_quantum;
extern int crash_qset;

extern int crash_p_buffer;	/* pipe.c */


#ifdef __KERNEL__
/*
 * Prototypes for shared functions
 */

int     crash_p_init(dev_t dev);
void    crash_p_cleanup(void);
int     crash_access_init(dev_t dev);
void    crash_access_cleanup(void);

int     crash_trim(struct crash_dev *dev);

ssize_t crash_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos);
ssize_t crash_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
loff_t  crash_llseek(struct file *filp, loff_t off, int whence);
long     crash_ioctl(struct file *filp,
                    unsigned int cmd, unsigned long arg);
#endif

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define CRASH_IOC_MAGIC  'k'
/* Please use a different 8-bit number in your code */

#define CRASH_ALLOC    _IO(CRASH_IOC_MAGIC, 0)
#define CRASH_USEAFTERFREE _IO(CRASH_IOC_MAGIC, 1)
#define CRASH_NULLPOINTER _IO(CRASH_IOC_MAGIC, 2)
#define CRASH_STACKOVERFLOW _IO(CRASH_IOC_MAGIC, 3)

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift": switch T and Q atomically
 */
#define CRASH_IOCSQUANTUM _IOW(CRASH_IOC_MAGIC,  1, int)
#define CRASH_IOCSQSET    _IOW(CRASH_IOC_MAGIC,  2, int)
#define CRASH_IOCTQUANTUM _IO(CRASH_IOC_MAGIC,   3)
#define CRASH_IOCTQSET    _IO(CRASH_IOC_MAGIC,   4)
#define CRASH_IOCGQUANTUM _IOR(CRASH_IOC_MAGIC,  5, int)
#define CRASH_IOCGQSET    _IOR(CRASH_IOC_MAGIC,  6, int)
#define CRASH_IOCQQUANTUM _IO(CRASH_IOC_MAGIC,   7)
#define CRASH_IOCQQSET    _IO(CRASH_IOC_MAGIC,   8)
#define CRASH_IOCXQUANTUM _IOWR(CRASH_IOC_MAGIC, 9, int)
#define CRASH_IOCXQSET    _IOWR(CRASH_IOC_MAGIC,10, int)
#define CRASH_IOCHQUANTUM _IO(CRASH_IOC_MAGIC,  11)
#define CRASH_IOCHQSET    _IO(CRASH_IOC_MAGIC,  12)

/*
 * The other entities only have "Tell" and "Query", because they're
 * not printed in the book, and there's no need to have all six.
 * (The previous stuff was only there to show different ways to do it.
 */
#define CRASH_P_IOCTSIZE _IO(CRASH_IOC_MAGIC,   13)
#define CRASH_P_IOCQSIZE _IO(CRASH_IOC_MAGIC,   14)
/* ... more to come */

#define CRASH_IOC_MAXNR 14

#endif /* _CRASH_H_ */
