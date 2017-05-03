
COMPILER_PREFIX ?= arm-none-eabi-

all: firefly

clean:
	rm -rf *.o
	rm firefly.*

boot:
	$(COMPILER_PREFIX)gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -c boot.S -o boot.o
init:
	$(COMPILER_PREFIX)gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -std=gnu99 -c kernel.c -o kernel.o -O2 -Wall -Wextra

firefly: boot init
	$(COMPILER_PREFIX)gcc -T linker.ld -o firefly.elf -ffreestanding -O2 -nostdlib boot.o kernel.o
