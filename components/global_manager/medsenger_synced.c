#include "medsenger_synced.h"
#include "freertos/idf_additions.h"
#include "init_global_manager_private.h"
#include "portmacro.h"
#include <assert.h>
#include <stdlib.h>

// medsenger_synced is true by default
// so that if any steps fails it will set to false
static bool gm_medsenger_synced = true;
static SemaphoreHandle_t gm_medsenger_synced_mutex;

void gm_init_medsenger_synced(void) {
    gm_medsenger_synced_mutex = xSemaphoreCreateMutex();
    assert(gm_medsenger_synced_mutex != NULL);
}

bool gm_get_medsenger_synced(void) {
    bool buf;
    if (xSemaphoreTake(gm_medsenger_synced_mutex, portMAX_DELAY) == pdTRUE) {
        buf = gm_medsenger_synced;
        xSemaphoreGive(gm_medsenger_synced_mutex);
    } else {
        abort();
    }
    return buf;
}

void gm_set_medsenger_synced(bool value) {
    if (xSemaphoreTake(gm_medsenger_synced_mutex, portMAX_DELAY) == pdTRUE) {
        gm_medsenger_synced = value;
        xSemaphoreGive(gm_medsenger_synced_mutex);
    } else {
        abort();
    }
}
