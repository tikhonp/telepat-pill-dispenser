#include "buzzer_player_private.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "pilld_common.h"
#include <stddef.h>

#define LEDC_OUTPUT_IO 5

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096

void b_buzzer_channel_init(void) {
    ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                          .channel = LEDC_CHANNEL,
                                          .timer_sel = LEDC_TIMER,
                                          .intr_type = LEDC_INTR_DISABLE,
                                          .gpio_num = LEDC_OUTPUT_IO,
                                          .duty = 0, // Set duty to 0%
                                          .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void play_note(uint32_t note, unsigned int duration_millis) {
    if (note == NOTE_EMPTY) {
        vTaskDelay(duration_millis / portTICK_PERIOD_MS);
        return;
    }
    ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                      .timer_num = LEDC_TIMER,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .freq_hz = b_notes[note],
                                      .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    vTaskDelay(duration_millis / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, LEDC_TIMER));
}

static void play_melody(const b_sample_t melody[], size_t melody_length) {
    for (int i = 0; i < melody_length; ++i)
        play_note(melody[i].note, melody[i].duration_millis);
}

void b_play_notification_task(enum b_notification_t notification) {
    switch (notification) {
    case FATAL_ERROR:
        play_melody(_b_fatal_err, ARR_LENGTH(_b_fatal_err));
        break;
    case PILL_NOTIFICATION:
        play_melody(_b_pill_not, ARR_LENGTH(_b_pill_not));
        break;
    }
}
