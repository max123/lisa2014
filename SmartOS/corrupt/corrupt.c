
/* Corrupt driver */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/cred.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "corrupt.h"

static int
corrupt_ioctl(dev_t dev, int cmd, intptr_t arg, int mode,
	      cred_t *cred_p, int *rval_p);
static int
corrupt_devinfo(dev_info_t *dip, ddi_info_cmd_t infocmd,
		void *arg, void **result);
static int
corrupt_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
static int
corrupt_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);

static dev_info_t *corrupt_dip; /* private devinfo pointer */

static struct module_info minfo = {
  0xee13,
  "corrupt",
  0,
  INFPSZ,
  512,
  128
};

#define CORRUPT_CONF_FLAG 

static struct cb_ops cb_corrupt_ops = {
   nulldev,               /* cb_open */
   nulldev,               /* cb_close */
   nodev,                 /* cb_strategy */
   nodev,                 /* cb_print */
   nodev,                 /* cb_dump */
   nodev,                 /* cb_read */
   nodev,                 /* cb_write */
   corrupt_ioctl,         /* cb_ioctl */
   nodev,                 /* cb_devmap */
   nodev,                 /* cb_mmap */
   nodev,                 /* cb_segmap */
   nochpoll,              /* cb_chpoll */
   ddi_prop_op,           /* cb_prop_op */
   NULL,                  /* cb_stream */
   (int)(D_NEW | D_MP)    /* cb_flag */
};

static struct dev_ops corrupt_ops = {
   DEVO_REV,                /* devo_rev */
   0,                       /* devo_refcnt */
   (corrupt_devinfo),          /* devo_getinfo */
   (nulldev),			/* devo_identify */
   (nulldev),               /* devo_probe */
   (corrupt_attach),           /* devo_attach */
   (corrupt_detach),           /* devo_detach */
   (nodev),                 /* devo_reset */
   &(cb_corrupt_ops),          /* devo_cb_ops */
   (struct bus_ops *)NULL,  /* devo_bus_ops */
   (int (*)()) NULL         /* devo_power */
};

/*
 * Module linkage information for the kernel.
 */

static struct modldrv modldrv = {
  &mod_driverops, "corrupt driver", &corrupt_ops
};

static struct modlinkage modlinkage = {
  MODREV_1, &modldrv, NULL
};

_init()
{
  return (mod_install(&modlinkage));
}

_info(modinfop)
  struct modinfo *modinfop;
{
  return (mod_info(&modlinkage, modinfop));
}

_fini(void)
{
  return (mod_remove(&modlinkage));
}

static int
corrupt_attach(dev_info_t *devi, ddi_attach_cmd_t cmd)
{
  if (cmd != DDI_ATTACH)
   return (DDI_FAILURE);

  if (ddi_create_minor_node(devi, "corruptmajor", S_IFCHR, 0, NULL, 0)
    == DDI_FAILURE) {
   ddi_remove_minor_node(devi, NULL);
   return (DDI_FAILURE);
  }

  corrupt_dip = devi;

  return (DDI_SUCCESS);
}

static int
corrupt_detach(dev_info_t *devi, ddi_detach_cmd_t cmd)
{
  if (cmd != DDI_DETACH)
   return (DDI_FAILURE);

  ddi_remove_minor_node(devi, NULL);
  return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
corrupt_devinfo(
  dev_info_t *dip,
  ddi_info_cmd_t infocmd,
  void *arg,
  void **result)
{
  int error;

  switch (infocmd) {
  case DDI_INFO_DEVT2DEVINFO:
   if (corrupt_dip == NULL) {
    error = DDI_FAILURE;
   } else {
    *result = (void *) corrupt_dip;
    error = DDI_SUCCESS;
   }
   break;
  case DDI_INFO_DEVT2INSTANCE:
   *result = (void *)0;
   error = DDI_SUCCESS;
   break;
  default:
   error = DDI_FAILURE;
  }
  return (error);
}

caddr_t crash_addr;

int
corrupt_ioctl(dev_t dev, int cmd, intptr_t arg, int mode,
	      cred_t *cred_p, int *rval_p)
{
	int retval = 0;

	switch(cmd) {

	case CRASH_ALLOC:
		crash_addr = kmem_alloc(256, KM_SLEEP);
		kmem_free(crash_addr, 256);
		break;

	case CRASH_USEAFTERFREE:
		memset(crash_addr, '?', 256);
		break;

	default:
		return ENOTTY;
	}

	return (retval);
}
