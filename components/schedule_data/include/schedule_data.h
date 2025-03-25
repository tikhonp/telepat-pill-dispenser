#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
/*
Schedule data for single pill cell.

So there is a defenition of a schedule data memory layout
single struct is for single cell.
One packet contains:
    - Start time for cell. uint32 timestamp (in seconds)
    - End time for cell.   uint32 timestamp (in seconds)
    - And single byte for meta data
SO there is 7 bytes per packet. Total data for all cells:

[ 4 bytes ][ 4 bytes ][ 1 byte ]|[ 4 bytes ][ 4 bytes ][ 1 byte ]|[ 4 bytes ][ 4 bytes ][ 1 byte ]|[ 4 
                                |                                |                                |       etc..
  0 indx packet -> cell 1       |  1 indx packet -> cell 2       |  2 indx packet -> cell 3       |
*/
typedef struct {
    uint32_t start_timestamp;
    uint32_t end_timestamp;
    uint8_t meta;
} sd_cell_schedule_t;
#pragma pack(pop)

void sd_init(void);

/*
Prints cell data for debug using ESP_LOGI.
*/
void sd_print_schedule(void);

/*
Saves new schedule in RAM and in FLASH, errors if data is invalid length
*/
esp_err_t sd_save_schedule(char *buf, unsigned int buf_length);

/*
Load schedule from flash called in case of failed to fetch it from network.
If err system must show global error to user.
*/
esp_err_t sd_load_schedule_from_flash(void);

/*
Get cell schedule for for specific cell indx
Must be called after sd_save_schedule() or sd_load_schedule_from_flash()
*/
sd_cell_schedule_t sd_get_schedule_by_cell_indx(int n);
