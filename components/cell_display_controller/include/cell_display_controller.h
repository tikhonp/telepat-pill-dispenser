#pragma once

#include "schedule_data.h"
#include <stdint.h>

typedef struct {
    sd_cell_schedule_t cell;
    uint8_t indx;
} cdc_cell_monitoring_t;

void cdc_run_cell_monitoring(void *params);
