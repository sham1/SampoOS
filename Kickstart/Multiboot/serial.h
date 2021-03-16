#pragma once

#include <stdbool.h>

bool serial_init(void);

void serial_putchar(char a);

void serial_write(const char *str);
