#pragma once

#include "esp_err.h"

typedef esp_err_t (*mhr_schedule_handler)(char *buf, unsigned int buf_length);

/*
Fetch schedule data from medsenger endpoit.
On data handler called if it returns an error request will be retied.
*/
esp_err_t mhr_fetch_schedule(mhr_schedule_handler h);
