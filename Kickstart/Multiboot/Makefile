.POSIX:
.SUFFIXES: .c .asm .o

CC = i686-elf-gcc
NASM = nasm

all: kickstart.bin

kickstart.bin: boot.o
	$(CC) -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

.asm.o:
	$(NASM) -felf32 $< -o $@
