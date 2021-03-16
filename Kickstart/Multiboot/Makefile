.POSIX:
.SUFFIXES: .c .asm .o

CC = i686-elf-gcc
CFLAGS = -ffreestanding -Wall -Wextra -O2 -std=gnu11 -g
NASM = nasm

OBJS =\
	boot.o \
	main.o \
	string.o \
	serial.o

all: kickstart.bin

kickstart.bin: crti.o crtbegin.o $(OBJS) crtend.o crtn.o
	$(CC) -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

string.o: string.c string.h
serial.o: serial.c serial.h

crtbegin.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtbegin.o` .

crtend.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtend.o` .

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.asm.o:
	$(NASM) -felf32 -F dwarf $< -o $@

clean:
	rm -f kickstart.bin *.o
