#pragma once

#include <stdint.h>

#define _str(a) #a
#define str(a) _str(a)

#define BITMASK(start, length) ((uint32_t)0xffffffff >> (32 - length)) << start

uint32_t decode_fixed32_little_end(const char *buffer);
