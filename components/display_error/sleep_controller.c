#include "sleep_controller.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "medsenger_refresh_rate.h"
#include "schedule_data.h"
#include "sdkconfig.h"
#include <stdint.h>
#include <sys/param.h>
#include <sys/time.h>

void de_sleep(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // find nearest cell
    uint32_t nearest_cell_time = 0;

    for (int i = 0; i < CONFIG_SD_CELLS_COUNT; ++i) {
        sd_cell_schedule_t cell = sd_get_schedule_by_cell_indx(i);
        if (cell.start_timestamp > tv.tv_sec &&
            (cell.start_timestamp < nearest_cell_time || nearest_cell_time == 0))
            nearest_cell_time = cell.start_timestamp;
    }

    uint32_t medsenger_refresh_rate;
    ESP_ERROR_CHECK(m_get_medsenger_refresh_rate_sec(&medsenger_refresh_rate));

    int wakeup_time_sec;
    if (nearest_cell_time != 0)
        wakeup_time_sec =
            MIN(medsenger_refresh_rate, nearest_cell_time - tv.tv_sec);
    else
        wakeup_time_sec = medsenger_refresh_rate;

    ESP_LOGI("SLEEP", "Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
    esp_deep_sleep_start();
}
