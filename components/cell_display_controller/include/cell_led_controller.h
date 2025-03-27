#pragma once

static const int cdc_cell_indx_to_gpio_map[] = {
    1,
    2,
    3,
    4,
};

void cdc_enable_signal(int indx);
void cdc_disable_signal(int indx);
