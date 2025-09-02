## APOLOGIES

This is an unholy mess

This is under series development.   It is an implementation of the ARM7-A CPU.

At the moment it's a SINGLE CPU.  it will be made multiple.
There is a nonzero or at least managable number of instructions which do work true to form.
However it is not complete.\\

The CPU does use a KEY12 decode for instructions.
It does use O(1) hashing with XMASK32 to assist for collisions.
The execute does build and use a 4096 table for O(1) lookup/use/dispatch.
THAT DOES MEAN there are collisions

This online version is only here because I am moving some things off my local machine as I am testing
on Windows/Linux/Mac. (insert emoji with touunge sticking out)

This will tie in with the vim-cmd project and the hostd project

vim-cmd will be the "Virtual Image Management" CMD that can be used to interface with hostd which will
be a daemon that can host Virtual machines.  If you seen VMWARE/Hyper-V/QEMU you have a good idea of what 
you will eventually be able to use this for.

BTW, Guppy will be a "fish" like utility that will be able to create VMs Disks; Set passwords; hostnames and
more than a few other things.  It also is a work in progress.  It's 2025 by 2035 this might be a thing.