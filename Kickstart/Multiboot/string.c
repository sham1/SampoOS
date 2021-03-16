#include "string.h"

void *
memmove(void *dest, const void *src, size_t count)
{
	unsigned char *destination = dest;
	const unsigned char *source = src;

	if (destination < source)
	{
		for (size_t i = 0; i < count; ++i)
		{
			destination[i] = source[i];
		}
	}
	else
	{
		for (size_t i = count; i != 0; --i)
		{
			destination[i-1] = source[i-1];
		}
	}

	return dest;
}

void *
memcpy(void *restrict dest, const void *restrict src, size_t count)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	for (size_t i = 0; i < count; ++i)
	{
		d[i] = s[i];
	}

	return dest;
}

void *
memset(void *dest, int ch, size_t count)
{
	unsigned char *d = dest;
	unsigned char c = ch;
	for (size_t i = 0; i < count; ++i)
	{
		d[i] = c;
	}

	return dest;
}

int
memcmp(const void *lhs, const void *rhs, size_t count)
{
	const unsigned char *a = lhs;
	const unsigned char *b = rhs;

	for (size_t i = 0; i < count; ++i)
	{
		if (a[i] < b[i])
		{
			return -1;
		}
		else if (a[i] > b[i])
		{
			return 1;
		}
	}

	return 0;
}

size_t
strlen(const char *str)
{
	const char *end;
	for (end = str; *end; ++end);
	return end - str;
}
