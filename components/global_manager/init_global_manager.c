#include "init_global_manager.h"
#include "init_global_manager_private.h"

void global_manager_init(void) {
    gm_init_medsenger_synced();
    gm_init_stop_all_tasks();
}
