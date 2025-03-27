#include "cell_display_controller.h"
#include "cell_led_controller.h"
#include "display_error.h"
#include "freertos/idf_additions.h"
#include "medsenger_synced.h"
#include "schedule_data.h"

void cdc_run_cell_monitoring(void *params) {
    cdc_cell_monitoring_t cell_data = *(cdc_cell_monitoring_t*)params;

    if (!gm_get_medsenger_synced() && !sd_get_processing_without_connection_allowed(&cell_data.cell)) {
        de_display_error(ACTIVE_CELL_WITH_FAILED_CONNECTION);
        vTaskDelete(NULL);
        return;
    }

    cdc_enable_signal(cell_data.indx);

}
