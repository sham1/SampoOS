ARCHDIR = Arch/$(ARCH)

CC = $(ARCH)-elf-gcc
CFLAGS = -ffreestanding -Wall -Wextra -std=gnu11 -O2 $(ARCH_CFLAGS)
LDFLAGS = -nostdlib -T $(ARCHDIR)/linker.ld
LIBS = -lgcc

OBJS =\
	main.o

sampo-$(ARCH).bin: $(ARCHDIR)/karch.a $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(ARCHDIR)/karch.a:
	make -C $(ARCHDIR) karch.a

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o sampo-$(ARCH).bin
	make -C $(ARCHDIR) clean
