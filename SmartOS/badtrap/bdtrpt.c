

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


/*
 * The entire state of each device.
 */
typedef struct {
	long		dummy[128];
	dev_info_t	*dip;		/* my devinfo handle */
} bdtrp_devstate_t;

static void *bdtrp_state;

static int bdtrp_open(dev_t *devp, int flag, int otyp, cred_t *cred);
static int bdtrp_read(dev_t dev, struct uio *uiop, cred_t *credp);
static int bdtrp_write(dev_t dev, struct uio *uiop, cred_t *credp);

static struct cb_ops bdtrp_cb_ops = {
	bdtrp_open,
	nulldev,	/* close */
	nodev,
	nodev,
	nodev,		/* dump */
	bdtrp_read,
	bdtrp_write,
	nodev,		/* ioctl */
	nodev,		/* devmap */
	nodev,		/* mmap */
	nodev,		/* segmap */
	nochpoll,	/* poll */
	ddi_prop_op,
	NULL,
	D_NEW | D_MP
};

static int bdtrp_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
    void **result);
static int bdtrp_attach(dev_info_t *dip, ddi_attach_cmd_t cmd);
static int bdtrp_detach(dev_info_t *dip, ddi_detach_cmd_t cmd);

static struct dev_ops bdtrp_ops = {
	DEVO_REV,
	0,
	bdtrp_getinfo,
	nulldev,	/* identify */
	nulldev,	/* probe */
	bdtrp_attach,
	bdtrp_detach,
	nodev,		/* reset */
	&bdtrp_cb_ops,
	(struct bus_ops *)0
};


extern struct mod_ops mod_driverops;

static struct modldrv modldrv = {
	&mod_driverops,
	"badtrap driver v1.0",
	&bdtrp_ops
};

static struct modlinkage modlinkage = {
	MODREV_1,
	&modldrv,
	0
};

extern int hz;

void
bdtrp_timer(void *arg)
{
	char *p;

	memset(p, '?', (int)arg);
	timeout(bdtrp_timer, arg, hz*120);
}

int
_init(void)
{
	int e;

	if ((e = ddi_soft_state_init(&bdtrp_state,
	    sizeof (bdtrp_devstate_t), 1)) != 0) {
		return (e);
	}

	if ((e = mod_install(&modlinkage)) != 0)  {
		ddi_soft_state_fini(&bdtrp_state);
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
	ddi_soft_state_fini(&bdtrp_state);
	return (e);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}

static int
bdtrp_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int instance;
	bdtrp_devstate_t *rsp;

	switch (cmd) {

	case DDI_ATTACH:

		instance = ddi_get_instance(dip);

		if (ddi_soft_state_zalloc(bdtrp_state, instance) != DDI_SUCCESS) {
			cmn_err(CE_CONT, "%s%d: can't allocate state\n",
			    ddi_get_name(dip), instance);
			return (DDI_FAILURE);
		} else
			rsp = ddi_get_soft_state(bdtrp_state, instance);

		if (ddi_create_minor_node(dip, "bdtrp", S_IFCHR,
		    instance, DDI_PSEUDO, 0) == DDI_FAILURE) {
			ddi_remove_minor_node(dip, NULL);
			goto attach_failed;
		}

		rsp->dip = dip;
		ddi_report_dev(dip);
		timeout(bdtrp_timer, (void *)4096, hz*120);
		return (DDI_SUCCESS);

	default:
		return (DDI_FAILURE);
	}

attach_failed:
	(void) bdtrp_detach(dip, DDI_DETACH);
	return (DDI_FAILURE);
}

static int
bdtrp_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int instance;
	register bdtrp_devstate_t *rsp;

	switch (cmd) {

	case DDI_DETACH:
		ddi_prop_remove_all(dip);
		instance = ddi_get_instance(dip);
		rsp = ddi_get_soft_state(bdtrp_state, instance);
		ddi_remove_minor_node(dip, NULL);
		ddi_soft_state_free(bdtrp_state, instance);
		return (DDI_SUCCESS);

	default:
		return (DDI_FAILURE);
	}
}

/*ARGSUSED*/
static int
bdtrp_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	bdtrp_devstate_t *rsp;
	int error = DDI_FAILURE;

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		if ((rsp = ddi_get_soft_state(bdtrp_state,
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


/*ARGSUSED*/
static int
bdtrp_open(dev_t *devp, int flag, int otyp, cred_t *cred)
{
	if (otyp != OTYP_BLK && otyp != OTYP_CHR)
		return (EINVAL);

	if (ddi_get_soft_state(bdtrp_state, getminor(*devp)) == NULL)
		return (ENXIO);

	return (0);
}


/*ARGSUSED*/
static int
bdtrp_read(dev_t dev, struct uio *uiop, cred_t *credp)
{
	int instance = getminor(dev);
	volatile dev_info_t *devinfop;
	bdtrp_devstate_t *rsp = NULL;

	rsp = ddi_get_soft_state(bdtrp_state, instance);
	devinfop = rsp->dip;
	return(0);

}

/*ARGSUSED*/
static int
bdtrp_write(dev_t dev, register struct uio *uiop, cred_t *credp)
{
	int instance = getminor(dev);
	bdtrp_devstate_t *rsp = ddi_get_soft_state(bdtrp_state, instance);
	return(0);
}

