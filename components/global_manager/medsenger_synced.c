#include "medsenger_synced.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdlib.h>

// medsenger_synced is true by default
// so that if any steps fails it will set to false
static bool medsenger_synced = true;
static SemaphoreHandle_t medsenger_synced_mutex;

void _init_medsenger_synced(void) {
    medsenger_synced_mutex = xSemaphoreCreateMutex();
    if (medsenger_synced_mutex == NULL) {
        abort();
    }
}

bool get_medsenger_synced(void) {
    bool buf;
    if (xSemaphoreTake(medsenger_synced_mutex, portMAX_DELAY) == pdTRUE) {
        buf = medsenger_synced;
        xSemaphoreGive(medsenger_synced_mutex);
    } else {
        abort();
    }
    return buf;
}

void set_medsenger_synced(bool value) {
    if (xSemaphoreTake(medsenger_synced_mutex, portMAX_DELAY) == pdTRUE) {
        medsenger_synced = value;
        xSemaphoreGive(medsenger_synced_mutex);
    } else {
        abort();
    }
}
