firefly: first it should be a static embeded OS followed OSEK/AUTOSAR specifications, which simulate in Qume with ARM architecture.

* Step 1 - install qemu

sudo apt-get install qemu-system-arm

* Step 2 - create virtual-machine for firefly OS

qemu-img create -f qcow2 ff.img 1G

* Step 3 - create a bootstrap for raspberry2

Ref: http://wiki.osdev.org/Raspberry_Pi_Bare_Bones

* Step 4 - build and run in qemu

make

qemu-system-arm -m 256 -M raspi2 -serial stdio -kernel firefly.elf

