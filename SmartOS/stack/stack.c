
/* stack driver for playing around */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "stack.h"

/*
 * The entire state of each stack device.
 */
typedef struct {
	dev_info_t	*dip;		/* my devinfo handle */
} stack_devstate_t;

/*
 * An opaque handle where our set of stack devices lives
 */
static void *stack_state;

static int stack_open(dev_t *devp, int flag, int otyp, cred_t *cred);
static int stack_read(dev_t dev, struct uio *uiop, cred_t *credp);
static int stack_write(dev_t dev, struct uio *uiop, cred_t *credp);
static int stack_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *cred_p, int *rval_p);

static struct cb_ops stack_cb_ops = {
	stack_open,
	nulldev,	/* close */
	nodev,
	nodev,
	nodev,		/* dump */
	stack_read,
	stack_write,
	stack_ioctl,		/* ioctl */
	nodev,		/* devmap */
	nodev,		/* mmap */
	nodev,		/* segmap */
	nochpoll,	/* poll */
	ddi_prop_op,
	NULL,
	D_NEW | D_MP
};

static int stack_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
    void **result);
static int stack_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int stack_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);

static struct dev_ops stack_ops = {
	DEVO_REV,
	0,
	stack_getinfo,
	nulldev,	/* identify */
	nulldev,	/* probe */
	stack_attach,
	stack_detach,
	nodev,		/* reset */
	&stack_cb_ops,
	(struct bus_ops *)0
};


extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,
	"stack driver v1.0",
	&stack_ops
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	0
};

int
_init(void)
{
	int e;

	if ((e = ddi_soft_state_init(&stack_state,
	    sizeof (stack_devstate_t), 1)) != 0) {
		return (e);
	}

	if ((e = mod_install(&modlinkage)) != 0)  {
		ddi_soft_state_fini(&stack_state);
	}

	return (e);
}

int
_fini(void)
{
	int e;

	if ((e = mod_remove(&modlinkage)) != 0)  {
		return (e);
	}
	ddi_soft_state_fini(&stack_state);
	return (e);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

static int
stack_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int instance;
	stack_devstate_t *rsp;

	switch (cmd) {

	case DDI_ATTACH:

		instance = ddi_get_instance(dip);

		if (ddi_soft_state_zalloc(stack_state, instance) != DDI_SUCCESS) {
			cmn_err(CE_CONT, "%s%d: can't allocate state\n",
			    ddi_get_name(dip), instance);
			return (DDI_FAILURE);
		} else
			rsp = ddi_get_soft_state(stack_state, instance);

		if (ddi_create_minor_node(dip, "stack", S_IFCHR,
		    instance, DDI_PSEUDO, 0) == DDI_FAILURE) {
			ddi_remove_minor_node(dip, NULL);
			goto attach_failed;
		}

		rsp->dip = dip;
		ddi_report_dev(dip);
		return (DDI_SUCCESS);

	default:
		return (DDI_FAILURE);
	}

attach_failed:
	(void) stack_detach(dip, DDI_DETACH);
	return (DDI_FAILURE);
}

static int
stack_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance;
	register stack_devstate_t *rsp;

	switch (cmd) {

	case DDI_DETACH:
		ddi_prop_remove_all(dip);
		instance = ddi_get_instance(dip);
		rsp = ddi_get_soft_state(stack_state, instance);
		ddi_remove_minor_node(dip, NULL);
		ddi_soft_state_free(stack_state, instance);
		return (DDI_SUCCESS);

	default:
		return (DDI_FAILURE);
	}
}

/*ARGSUSED*/
static int
stack_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	stack_devstate_t *rsp;
	int error = DDI_FAILURE;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if ((rsp = ddi_get_soft_state(stack_state,
		    getminor((dev_t)arg))) != NULL) {
			*result = rsp->dip;
			error = DDI_SUCCESS;
		} else
			*result = NULL;
		break;

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)getminor((dev_t)arg);
		error = DDI_SUCCESS;
		break;

	default:
		break;
	}

	return (error);
}

int firstopen = 1;

/* stack_function should get segkp redzone access panic */
void
stack_function(void)
{
  char bigstack[1024*32];
  char *p;

  for (p = &bigstack[(1024*32)-1]; p > bigstack; p--)
    *p = '?';
}

/* stack_corrupt results in corrupted data (stack overflow by-passes redzone page) */
void
stack_corrupt(void)
{
  char bigstack[1024*128];
  char *p;

  for (p = bigstack; p < &bigstack[8192]; p--)
    *p = '?';

}

/*ARGSUSED*/
static int
stack_open(dev_t *devp, int flag, int otyp, cred_t *cred)
{
	if (otyp != OTYP_BLK && otyp != OTYP_CHR)
		return (EINVAL);

	if (ddi_get_soft_state(stack_state, getminor(*devp)) == NULL)
		return (ENXIO);

	return (0);
}


/*ARGSUSED*/
static int
stack_read(dev_t dev, struct uio *uiop, cred_t *credp)
{
	int instance = getminor(dev);
	stack_devstate_t *rsp = ddi_get_soft_state(stack_state, instance);
	return(0);

}

/*ARGSUSED*/
static int
stack_write(dev_t dev, register struct uio *uiop, cred_t *credp)
{
	int instance = getminor(dev);
	stack_devstate_t *rsp = ddi_get_soft_state(stack_state, instance);
	return(0);
}

/*ARGSUSED*/
static int
stack_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
    switch(cmd) {
    case REDZONE_OVERFLOW:
	stack_function();
	return 0;
    case CORRUPT_OVERFLOW:
	stack_corrupt();
	return 0;
    }
}
