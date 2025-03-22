#include "schedule_data.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <assert.h>

const char *TAG = "cell-schedule-data";

void sd_print_cell_schedule(sd_cell_schedule_t cs) {
    ESP_LOGI(TAG, "SC[%" PRIu32 ", %" PRIu32 ", %" PRIu8 "]",
             cs.start_timestamp, cs.end_timestamp, cs.meta);
}

sd_cell_schedule_t *sd_parse_schedule(char *buf, unsigned int buf_length) {
    assert((buf_length / sizeof(sd_cell_schedule_t)) == CONFIG_SD_CELLS_COUNT);
    return (sd_cell_schedule_t *)buf;
}
