#include "serial.h"
#include "util.h"
#include <stdarg.h>

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

static void
itoa(char *buffer, int base, int num)
{
	char *p = buffer;
	unsigned long d = num;
	if (base == 10 && num < 0)
	{
		*p++ = '-';
		++buffer;
		d = -num;
	}

	int printed_chars = 0;
	bool should_quit = false;
	do
	{
		int rem = d % base;
		*p++ = (rem < 10) ? rem + '0' : rem + 'a' - 10;
		++printed_chars;
		d /= base;

		switch (base)
		{
		case 10:
			should_quit = d == 0;
			break;
		case 16:
			should_quit = printed_chars == 8;
			break;
		default:
			should_quit = true;
			break;
		}
	}
	while (!should_quit);
	*p = '\0';

	char *p1 = buffer;
	char *p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		++p1;
		--p2;
	}
}

void
serial_printf(const char *restrict format, ...)
{
	va_list args;
	va_start(args, format);

	for (const char *iter = format; *iter; ++iter)
	{
		if (*iter == '%')
		{
			++iter;
			if (*iter == '\0')
			{
				break;
			}

			char buffer[21];

		        switch(*iter)
			{
			case '%':
				serial_putchar('%');
				break;
			case 's':
			{
				const char *s = va_arg(args, const char *);
				serial_write(s);
				break;
			}
			case 'c':
			{
				int c = va_arg(args, int);
				serial_putchar(c);
				break;
			}
			case 'x':
			{
				unsigned int n = va_arg(args, unsigned int);
				itoa(buffer, 16, n);
				serial_write(buffer);
				break;
			}
			case 'd':
			{
				int n = va_arg(args, int);
				itoa(buffer, 10, n);
				serial_write(buffer);
				break;
			}
			case 'u':
			{
				unsigned int n = va_arg(args, unsigned int);
				itoa(buffer, 10, n);
				serial_write(buffer);
				break;
			}
			}
		}
		else
		{
			serial_putchar(*iter);
		}
	}

	va_end(args);
}
