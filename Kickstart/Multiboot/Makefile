.POSIX:
.SUFFIXES: .c .asm .o

CC = i686-elf-gcc
CFLAGS =
NASM = nasm

OBJS =\
	boot.o

all: kickstart.bin

kickstart.bin: crti.o crtbegin.o $(OBJS) crtend.o crtn.o
	$(CC) -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

crtbegin.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtbegin.o` .

crtend.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtend.o` .

.asm.o:
	$(NASM) -felf32 $< -o $@

clean:
	rm -f kickstart.bin *.o
