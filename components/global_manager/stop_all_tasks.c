#include "stop_all_tasks.h"
#include "freertos/idf_additions.h"
#include "init_global_manager_private.h"

static bool gm_stop_all_tasks_fired = false;
static SemaphoreHandle_t gm_stop_all_tasks_fired_mu;

void gm_init_stop_all_tasks(void) {
    gm_stop_all_tasks_fired_mu = xSemaphoreCreateMutex();
    if (gm_stop_all_tasks_fired_mu == NULL) {
        abort();
    }
}

bool gm_is_stop_all_tasks_fired(void) {
    bool buf;
    if (xSemaphoreTake(gm_stop_all_tasks_fired_mu, portMAX_DELAY) == pdTRUE) {
        buf = gm_stop_all_tasks_fired_mu;
        xSemaphoreGive(gm_stop_all_tasks_fired_mu);
    } else {
        abort();
    }
    return buf;
}

void gm_fire_stop_all_tasks(void) {
    if (xSemaphoreTake(gm_stop_all_tasks_fired_mu, portMAX_DELAY) == pdTRUE) {
        gm_stop_all_tasks_fired = true;
        xSemaphoreGive(gm_stop_all_tasks_fired_mu);
    } else {
        abort();
    }
}
