#include "serial.h"
#include "util.h"

#define COM1 0x3f8

bool
serial_init(void)
{
	outb(COM1 + 1, 0x00); // Disable interrupts
	outb(COM1 + 3, 0x80); // Enable DLAB to set baud rate divisor
	outb(COM1 + 0, 0x03); // Set low byte of divisor to 3.
	outb(COM1 + 1, 0x00); // Set hight byte of divisor to 0. I.e. divisor is 3.
	outb(COM1 + 3, 0x03); // Set mode: 8-bits, no parity and one stop-bit
	outb(COM1 + 2, 0xC7); // Enable FIFO, and clear them. Set 14-byte threshold.
	outb(COM1 + 4, 0x0B); // Enable IRQs, set RTS/DST
	outb(COM1 + 4, 0x1E); // Set in loopback mode for self-test.
	outb(COM1 + 0, 0xAE); // Test the serial by sending and then receiving.

	if (inb(COM1 + 0) != 0xAE)
	{
		// Self-test failed.
		return false;
	}

	// Self-test successful. Set normal operation mode, so no
	// loopback, but with IRQs and OUT#1 and OUT#2 -bits enabled.
	outb(COM1 + 4, 0x0F);
	return true;
}

static bool
is_transmit_empty(void)
{
	return (inb(COM1 + 5) & 0x20) != 0;
}

void
serial_putchar(char a)
{
	while (!is_transmit_empty());

	outb(COM1, a);
}

void
serial_write(const char *str)
{
	while (*str)
	{
		serial_putchar(*str++);
	}
}
