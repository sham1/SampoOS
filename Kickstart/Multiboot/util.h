#pragma once

#include <stdint.h>

static inline void
outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void
outw(uint16_t port, uint16_t val)
{
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void
outl(uint16_t port, uint32_t val)
{
    asm volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline uint16_t
inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline uint32_t
inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline uint16_t
read_u16_le(const uint8_t *bytes)
{
    uint16_t val = 0;
    val |= (((uint16_t)bytes[0]) << 0);
    val |= (((uint16_t)bytes[1]) << 8);

    return val;
}

static inline uint16_t
read_u16_be(const uint8_t *bytes)
{
    uint16_t val = 0;

    val |= (((uint16_t)bytes[0]) << 8);
    val |= (((uint16_t)bytes[1]) << 0);

    return val;
}

static inline uint32_t
read_u32_le(const uint8_t *bytes)
{
    uint32_t val = 0;
    val |= (((uint32_t)bytes[0]) << 0);
    val |= (((uint32_t)bytes[1]) << 8);
    val |= (((uint32_t)bytes[2]) << 16);
    val |= (((uint32_t)bytes[3]) << 24);

    return val;
}

static inline uint32_t
read_u32_be(const uint8_t *bytes)
{
    uint32_t val = 0;

    val |= (((uint32_t)bytes[0]) << 24);
    val |= (((uint32_t)bytes[1]) << 16);
    val |= (((uint32_t)bytes[2]) << 8);
    val |= (((uint32_t)bytes[3]) << 0);

    return val;
}

static inline uint64_t
read_u64_le(const uint8_t *bytes)
{
    uint64_t val = 0;
    val |= (((uint64_t)bytes[0]) << 0);
    val |= (((uint64_t)bytes[1]) << 8);
    val |= (((uint64_t)bytes[2]) << 16);
    val |= (((uint64_t)bytes[3]) << 24);
    val |= (((uint64_t)bytes[4]) << 32);
    val |= (((uint64_t)bytes[5]) << 40);
    val |= (((uint64_t)bytes[6]) << 48);
    val |= (((uint64_t)bytes[7]) << 56);

    return val;
}

static inline uint64_t
read_u64_be(const uint8_t *bytes)
{
    uint64_t val = 0;

    val |= (((uint64_t)bytes[0]) << 56);
    val |= (((uint64_t)bytes[1]) << 48);
    val |= (((uint64_t)bytes[2]) << 40);
    val |= (((uint64_t)bytes[3]) << 32);
    val |= (((uint64_t)bytes[4]) << 24);
    val |= (((uint64_t)bytes[5]) << 16);
    val |= (((uint64_t)bytes[6]) << 8);
    val |= (((uint64_t)bytes[7]) << 0);

    return val;
}
