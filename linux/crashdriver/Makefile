# Comment/uncomment the following line to disable/enable debugging
DEBUG = y


# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DCRASH_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

ccflags-y += $(DEBFLAGS)
ccflags-y += -I$(LDDINC)
ccflags-y += -imacros /home/max/linux-3.16.3/include/generated/autoconf.h
ccflags-y += -I /home/max/linux-3.16.3/include

ifneq ($(KERNELRELEASE),)
# call from kernel build system

crash-objs := main.o

obj-m	:= crash.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules

endif


crashapp: crashapp.c
	$(CC) -O crashapp.c -o crashapp

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
