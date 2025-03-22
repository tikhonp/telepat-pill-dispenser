#pragma once

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

/*
Prints cell data for debug using ESP_LOGI.
*/
void sd_print_cell_schedule(sd_cell_schedule_t cs);

/*
Converts array of bytes to array of cell schedules structs. 
Returns pointer to same buffer just length check and cast under the hood. Output array size is CONFIG_SD_CELLS_COUNT
*/
sd_cell_schedule_t *sd_parse_schedule(char *buf, unsigned int buf_length);
