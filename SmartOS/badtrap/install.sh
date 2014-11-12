#! /bin/ksh
cp i386/obj32/bdtrp /usr/kernel/drv/bdtrp
cp i386/obj64/bdtrp /usr/kernel/drv/bdtrp
cp bdtrp.conf /usr/kernel/drv/bdtrp.conf
add_drv bdtrp
cat /devi*/pse*/bdtrp*
