#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "hal/ledc_types.h"
#include "sdkconfig.h"
#include <stdint.h>

#define ZERO_CELL_PIN 1

static const char *TAG = "CELLS";

const uint16_t notes[] = {
    0,    31,   33,   35,   37,   39,   41,   44,   46,   49,   52,   55,
    58,   62,   65,   69,   73,   78,   82,   87,   93,   98,   104,  110,
    117,  123,  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,
    233,  247,  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,
    466,  494,  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,
    932,  988,  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760,
    1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520,
    3729, 3951, 4186, 4435, 4699, 4978};

typedef enum {
    NO_NOTE = 0,
    NOTE_A0,
    NOTE_AS0,
    NOTE_B0,
    NOTE_C1,
    NOTE_CS1,
    NOTE_D1,
    NOTE_DS1,
    NOTE_E1,
    NOTE_F1,
    NOTE_FS1,
    NOTE_G1,
    NOTE_GS1,
    NOTE_A1,
    NOTE_AS1,
    NOTE_B1,
    NOTE_C2,
    NOTE_CS2,
    NOTE_D2,
    NOTE_DS2,
    NOTE_E2,
    NOTE_F2,
    NOTE_FS2,
    NOTE_G2,
    NOTE_GS2,
    NOTE_A2,
    NOTE_AS2,
    NOTE_B2,
    NOTE_C3,
    NOTE_CS3,
    NOTE_D3,
    NOTE_DS3,
    NOTE_E3,
    NOTE_F3,
    NOTE_FS3,
    NOTE_G3,
    NOTE_GS3,
    NOTE_A3,
    NOTE_AS3,
    NOTE_B3,
    NOTE_C4,
    NOTE_CS4,
    NOTE_D4,
    NOTE_DS4,
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4,
    NOTE_G4,
    NOTE_GS4,
    NOTE_A4,
    NOTE_AS4,
    NOTE_B4,
    NOTE_C5,
    NOTE_CS5,
    NOTE_D5,
    NOTE_DS5,
    NOTE_E5,
    NOTE_F5,
    NOTE_FS5,
    NOTE_G5,
    NOTE_GS5,
    NOTE_A5,
    NOTE_AS5,
    NOTE_B5,
    NOTE_C6,
    NOTE_CS6,
    NOTE_D6,
    NOTE_DS6,
    NOTE_E6,
    NOTE_F6,
    NOTE_FS6,
    NOTE_G6,
    NOTE_GS6,
    NOTE_A6,
    NOTE_AS6,
    NOTE_B6,
    NOTE_C7,
    NOTE_CS7,
    NOTE_D7,
    NOTE_DS7,
    NOTE_E7,
    NOTE_F7,
    NOTE_FS7,
    NOTE_G7,
    NOTE_GS7,
    NOTE_A7,
    NOTE_AS7,
    NOTE_B7,
    NOTE_C8,
} piano_note_t;

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (5) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)      // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000) // Frequency in Hertz. Set frequency at 4 kHz

void buzzer_channel_init() {
    ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                          .channel = LEDC_CHANNEL,
                                          .timer_sel = LEDC_TIMER,
                                          .intr_type = LEDC_INTR_DISABLE,
                                          .gpio_num = LEDC_OUTPUT_IO,
                                          .duty = 0, // Set duty to 0%
                                          .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void tone(uint32_t freq, int duration_sec) {
    ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                      .timer_num = LEDC_TIMER,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .freq_hz = freq,
                                      .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    vTaskDelay(duration_sec * 1000 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(ledc_timer_pause(LEDC_MODE, LEDC_TIMER));
}

void init_cells() {
    for (int i = 0; i < CONFIG_CELLS_COUNT; i++) {
        gpio_reset_pin(ZERO_CELL_PIN + i);
        gpio_set_direction(ZERO_CELL_PIN + i, GPIO_MODE_OUTPUT);
    }
    buzzer_channel_init();
}

void enable_cell(int indx) {
    if (indx >= CONFIG_CELLS_COUNT) {
        ESP_LOGE(TAG, "Invalid cell indx");
        abort();
    }
    gpio_set_level(ZERO_CELL_PIN + indx, 1);
    tone(NOTE_D7, 1);
}

void disable_cell(int indx) {
    if (indx >= CONFIG_CELLS_COUNT) {
        ESP_LOGE(TAG, "Invalid cell indx");
        abort();
    }
    gpio_set_level(ZERO_CELL_PIN + indx, 0);
}
