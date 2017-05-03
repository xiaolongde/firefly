firefly: first it should be a static embeded OS followed OSEK/AUTOSAR specifications, which simulate in Qume with ARM architecture.

* Step 1 - install qemu

	sudo apt-get install qemu-system-arm

* Step 2 - create virtual-machine for firefly OS

	qemu-img create -f qcow2 ff.img 1G

* Step 3 - create a bootstrap for raspberry2

	Ref: http://wiki.osdev.org/Raspberry_Pi_Bare_Bones
	
	Ref: https://github.com/idrawone/qemu-rpi

* Step 4 - build and run in qemu

	make
	
	qemu-system-arm -m 128 -M raspi2 -s -S -nographic -kernel firefly.elf

* Step 5 - debug step by step using gdb
	
	arm-none-eabi-gdb
	
	target remote localhost:1234
	
	continue
	
	file firefly.elf
	
	continue
	
	blabla...
