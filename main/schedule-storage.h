#pragma once

#include <stdint.h>

void init_schedule();
void update_schedule(uint32_t *timestamps);
uint32_t schedule_get(int i);
