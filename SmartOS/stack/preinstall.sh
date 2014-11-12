#! /bin/ksh
cp i386/obj32/stack /usr/kernel/drv/stack
cp stack.conf /usr/kernel/drv/stack.conf
cp i386/obj32/stacktest /stacktest
rem_drv stack
add_drv stack
