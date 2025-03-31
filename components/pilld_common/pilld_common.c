#include "pilld_common.h"

uint32_t decode_fixed32_little_end(const char *buffer) {
    return ((uint32_t)(buffer[0])) | ((uint32_t)(buffer[1]) << 8) |
           ((uint32_t)(buffer[2]) << 16) | ((uint32_t)(buffer[3]) << 24);
}
