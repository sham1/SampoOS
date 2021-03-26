ARCHDIR = Arch/$(ARCH)

CC = $(ARCH)-elf-gcc
CFLAGS = -ffreestanding -Wall -Wextra -std=gnu11 -O2 -g -I../Kickstart/include -Iinclude $(ARCH_CFLAGS)
LDFLAGS = -nostdlib -T $(ARCHDIR)/linker.ld $(ARCH_LDFLAGS)
LIBS = -lgcc
NASM = nasm
NASMFLAGS = $(ARCH_NASMFLAGS)

all: sampo-$(ARCH).bin

include $(ARCHDIR)/make.config

OBJS =\
	$(ARCHDIR)/crti.o \
	$(ARCHDIR)/crtbegin.o \
	main.o \
	$(ARCH_OBJS) \
	$(ARCHDIR)/crtend.o \
	$(ARCHDIR)/crtn.o \

sampo-$(ARCH).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o sampo-$(ARCH).bin $(ARCHDIR)/*.o

.SUFFIXES: .c .asm .o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

.asm.o:
	$(NASM) $(NASMFLAGS) $< -o $@

$(ARCHDIR)/crtbegin.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtbegin.o` $(ARCHDIR)

$(ARCHDIR)/crtend.o:
	cp `$(CC) $(CFLAGS) -print-file-name=crtend.o` $(ARCHDIR)
