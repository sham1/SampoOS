.POSIX:

CC = x86_64-elf-gcc
CFLAGS = -ffreestanding -Wall -Wextra -O2 -std=gnu11 $(ARCH_CFLAGS)
NASM = nasm
AR = x86_64-elf-ar

OBJS =\
	start.o

all: karch.a

karch.a: crti.o crtbegin.o $(OBJS) crtend.o crtn.o
	$(AR) rs $@ $^

crtbegin.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtbegin.o` .

crtend.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtend.o` .

.SUFFIXES: .c .o .asm

.asm.o:
	$(NASM) -felf64 $< -o $@
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f karch.a *.o
