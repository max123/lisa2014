# LISA 2014 Kernel Debugging Tutorial Materials#

This contains the slides, lab files, and lab directions for the LISA 2014 tutorial on kernel debugging.

Those planning to attend the tutorial at LISA 2014 on kernel debugging will need to do some prep work prior to attending.
This repo will also serve as a place to hold the files needed for following along during the tutorial.

To follow along, you'll need to install (and test!) the following:

* A SmartOS system.
* 3 Linux systems.
* A Freebsd system.

Each machine should have 2GB of memory (less may work, more should not be needed), with 40GB of disk space (not all systems
need as much).  If I have time, I shall make vagrant images of each system.  If anyone else wants to do that, please do and let me know.

The following covers some details for each of the systems.

SmartOS
=======

On VirtualBox, VMware, (and parallels?), follow the directions at [http://wiki.smartos.org/display/DOC/Getting+Started+with+SmartOS](http://wiki.smartos.org/display/DOC/Getting+Started+with+SmartOS).
While this is probably sufficient for doing the labs, if you want to use the machine for more, you shoud then follow instructions at [http://wiki.smartos.org/display/DOC/How+to+create+a+zone+%28+OS+virtualized+machine+%29+in+SmartOS](http://wiki.smartos.org/display/DOC/How+to+create+a+zone+%28+OS+virtualized+machine+%29+in+SmartOS).


Linux
=======

You'll need two boxes for running kgdb/kdb, probably easiest if they
are both identical.  And one of the two boxes will also be used for
doing kernel crash dump analysis.
Though the techniques covered in the tutorial should work with any version of linux, I have only tried with Ubuntu.
You can get the latest ubuntu image by going to
[http://www.ubuntu.com/download/server](http://www.ubuntu.com/download/server).

To use kgdb/kdb, you'll need to do the following:

* Once you have installed linux, you'll need to download and build
  from source.  On the Ubuntu server I am running, I used:
  

	$ curl -O  https://www.kernel.org/pub/linux/kernel/v3.x/linux-3.16.3.tar.
	$ tar xvf linux-3.16.3.tar.xz

to download and unpack the code.
Then follow the instructions at
[http://bipinkunal.blogspot.com/2012/05/kgdb-tutorial.html](http://bipinkunal.blogspot.com/2012/05/kgdb-tutorial.html).

These are reasonably complete.  I found that to get the target and
host (debuggee and debugger) to communicate over /dev/ttyS0 (serial
port) on Fusion VMware, I had to add lines to the .xml files for my
two machines.  For the target, the file will be the name you give your
virtual machine (from the "Virtual Machine Library" window, (though
I'm sure you can find this out at the command line)) appended with
".vmx".  So, on my Mac, the file for the target is `~/Documents/Virtual
Machines.localized/Ubuntu 64-bit kgdb target.vmwarevm/Ubuntu 64-bit
kgdb target.vmx`.  It may be in a completely different location on
your machine.  "Ubuntu 64-bit kgdb target" was the name I gave the
virtual machine when it was created.

For the target (system being debugged), I added the following lines (deleting any lines containing "serial1."
prior to doing this):

	serial1.present = "TRUE"
	serial1.fileType = "pipe"
	serial1.yieldOnMsrRead = "TRUE"
	serial1.startConnected = "TRUE"
	serial1.pipe.endPoint = "client"
	serial1.fileName = "/tmp/serial"

For the host (system running the debugger), I added the following lines (deleting any lines containing "serial1."
prior to doing this):

	serial1.present = "TRUE"
	serial1.fileType = "pipe"
	serial1.yieldOnMsrRead = "TRUE"
	serial1.startConnected = "TRUE"
	serial1.pipe.endPoint = "server"
	serial1.fileName = "/tmp/serial"

As far as I have seen, it doesn't matter which you make server or
client.  The .vmx file for my host (debugger) vm  is at `~/Documents/Virtual
Machines.localized/Ubuntu 64-bit debug host.vmwarevm/Ubuntu 64-bit
debug host.vmx`.  You'll know that this setup is working if you can
reboot the target and, from the host, run:

	$ sudo 

For examining kernel crash dumps on linux, you'll need to find a way to get
the system to create a dump in the first place.  On Ubuntu systems,
you can follow the steps at
[https://wiki.ubuntu.com/Kernel/CrashdumpRecipe](https://wiki.ubuntu.com/Kernel/CrashdumpRecipe).
You can do these on the host (debugger) machine.  It might also work
on the target, but I think that kdb on that machine is enough.  Make
sure you do the part about getting a vmlinux file with debug
information in the "Using crash" section.  If you follow all
instructions in the CrashdumpRecipe page, you should be able to:

	$ sudo crash /usr/lib/debug/boot/vmlinux-3.13.0-32-generic /var/crash/201410080827/dump.201410080827 
	crash 7.0.3
	Copyright (C) 2002-2013  Red Hat, Inc.
	Copyright (C) 2004, 2005, 2006, 2010  IBM Corporation
	Copyright (C) 1999-2006  Hewlett-Packard Co
	Copyright (C) 2005, 2006, 2011, 2012  Fujitsu Limited
	Copyright (C) 2006, 2007  VA Linux Systems Japan K.K.
	Copyright (C) 2005, 2011  NEC Corporation
	Copyright (C) 1999, 2002, 2007  Silicon Graphics, Inc.
	Copyright (C) 1999, 2000, 2001, 2002  Mission Critical Linux, Inc.
	This program is free software, covered by the GNU General Public License,
	and you are welcome to change it and/or distribute copies of it under
	certain conditions.  Enter "help copying" to see the conditions.
	This program has absolutely no warranty.  Enter "help warranty" for details.
	 
	GNU gdb (GDB) 7.6
	Copyright (C) 2013 Free Software Foundation, Inc.
	License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
	This is free software: you are free to change and redistribute it.
	There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
	and "show warranty" for details.
	This GDB was configured as "x86_64-unknown-linux-gnu"...
	
	      KERNEL: /usr/lib/debug/boot/vmlinux-3.13.0-32-generic
	    DUMPFILE: dump.201410080827  [PARTIAL DUMP]
	        CPUS: 1
	        DATE: Wed Oct  8 08:27:33 2014
	      UPTIME: 00:03:53
	LOAD AVERAGE: 0.03, 0.04, 0.03
	       TASKS: 214
	    NODENAME: ubuntu
	     RELEASE: 3.13.0-32-generic
	     VERSION: #57-Ubuntu SMP Tue Jul 15 03:51:08 UTC 2014
	     MACHINE: x86_64  (2393 Mhz)
	      MEMORY: 4 GB
	       PANIC: "Oops: 0002 [#1] SMP " (check log for details)
	         PID: 1295
	     COMMAND: "bash"
	        TASK: ffff880036412fe0  [THREAD_INFO: ffff88003653a000]
	         CPU: 0
	       STATE: TASK_RUNNING (PANIC)
	
	crash> quit
	$

where `/usr/lib/debug/boot/vmlinux-3.13.0-32-generic` is the debug
kernel, and `/var/crash/201410080827/dump.201410080827`  is the crash
dump.


This repo is a work in progress.


