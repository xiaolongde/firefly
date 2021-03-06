
COMPILER_PREFIX ?= arm-none-eabi-

AS_CLFAGS ?= -mcpu=arm1176jzf-s -fpic -ffreestanding
C_CFLAGS ?= -mcpu=arm1176jzf-s -g3 -Wextra -fpic -Wall -ffreestanding -std=gnu99 -O2 -Wall -Wextra
LIBS ?= -L/usr/lib/arm-none-eabi/lib/ -L/usr/lib/gcc/arm-none-eabi/4.8.2/

OBJS = boot.o 
OBJS += kernel.o 

all: firefly

clean:
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img

%.o : %.S
	$(COMPILER_PREFIX)gcc $(AS_CLFAGS) -D__ASSEMBLY__ -c -o $@ $<
	
%.o : %.c
	$(COMPILER_PREFIX)gcc $(C_CFLAGS) -c -o $@ $<

boot:
	$(COMPILER_PREFIX)gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -c boot.S -o boot.o
init:
	$(COMPILER_PREFIX)gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -std=gnu99 -c kernel.c -o kernel.o -O2 -Wall -Wextra

firefly: linker.ld $(OBJS)
	$(COMPILER_PREFIX)gcc -T linker.ld -o firefly.elf -ffreestanding -O2 -nostdlib $(OBJS) -lc -lgcc
	$(COMPILER_PREFIX)objdump -D firefly.elf > firefly.list
	$(COMPILER_PREFIX)objcopy firefly.elf -O ihex firefly.hex
	$(COMPILER_PREFIX)objcopy firefly.elf -O binary firefly.bin
	$(COMPILER_PREFIX)objcopy firefly.elf -O binary firefly.img

qemu-gdb:
	qemu-system-arm -m 128 -M raspi2 -s -S -nographic -kernel firefly.elf
qemu:
	qemu-system-arm -m 128 -M raspi2 -s -nographic -kernel firefly.elf
