#include "display_error.h"
#include "stop_all_tasks.h"
#include "buzzer.h"


void display_error(de_fatal_error_t error) {
    gm_fire_stop_all_tasks();

    // TODO: Light here

    b_play_notification(FATAL_ERROR);

    // wait for button press
    
    // reset
}
