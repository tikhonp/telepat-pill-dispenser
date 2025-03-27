#include "display_error.h"
#include "button_controller.h"
#include "buzzer.h"
#include "esp_system.h"
#include "led_controller.h"
#include "stop_all_tasks.h"

void de_display_error(de_fatal_error_t error) {
    gm_fire_stop_all_tasks();

    de_start_blinking();

    b_play_notification(FATAL_ERROR);

    bc_wait_for_single_press();

    esp_restart();
}
