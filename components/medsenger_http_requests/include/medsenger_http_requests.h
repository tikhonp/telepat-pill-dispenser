#pragma once

#include "esp_err.h"

typedef esp_err_t (*mhr_schedule_handler)(char *buf, unsigned int buf_length);

/*
Fetch schedule data from medsenger endpoit.
On data handler called if it returns an error request will be retied.
*/
esp_err_t mhr_fetch_schedule(mhr_schedule_handler h);

/*
Submits succes request for specific cell to medsenger.
*/
esp_err_t mhr_submit_succes_cell(uint32_t timestamp, uint8_t cell_indx);
