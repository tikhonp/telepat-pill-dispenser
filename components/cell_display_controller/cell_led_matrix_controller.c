#include "sdkconfig.h"
#include <assert.h>
#include <stdint.h>
#ifdef CONFIG_CDC_LEDS_MATRIX
#include "cell_led_controller.h"
#include <esp_log.h>
#include <ht16k33.h>
#include <i2cdev.h>
#include <string.h>

/*
* So if  matrix is 4x7 where 4-`COM`s and 7 is `a`
*
* It whould look like this
*
*      com0 com1 com2 com3
* row0 -    -    -    -    byte0 [0, 0, 0, 0, unused, unused, unused, unused] system idxs 0-3
* row1 -    -    -    -    byte1 [0, 0, 0, 0, unused, unused, unused, unused]             4-7
* row2 -    -    -    -    byte2 [0, 0, 0, 0, unused, unused, unused, unused]             8-11
* row3 -    -    -    -    byte3 [0, 0, 0, 0, unused, unused, unused, unused]             12-15
* row4 -    -    -    -    byte4 [0, 0, 0, 0, unused, unused, unused, unused]             16-19
* row5 -    -    -    -    byte5 [0, 0, 0, 0, unused, unused, unused, unused]             20-23
* row6 -    -    -    -    byte0 [0, 0, 0, 0, unused, unused, unused, unused]             24-27
*
* There is byte array `leds_state`
*
* And if i want set let's say (2, 4) led (com1, row3) i need to set second bit of byte3
* 
* So i count system cell indexes from left to right and from top to bottom
*/

static char *TAG = "leds-matrix-controller";

static i2c_dev_t dev;
static uint8_t leds_state[HT16K33_RAM_SIZE_BYTES] = {0};

static SemaphoreHandle_t leds_mu;

static void enable_signal_in_leds_state(uint8_t indx) {
    uint8_t byte_indx;
    if (indx > 4) {
        byte_indx = (indx / 4) - 1;
    } else {
        byte_indx = 0;
    }
    uint8_t bit_indx = indx % 4;
    leds_state[byte_indx] = leds_state[byte_indx] | (1 << bit_indx);
}

static void disable_signal_in_leds_state(uint8_t indx) {
    uint8_t byte_indx;
    if (indx > 4) {
        byte_indx = (indx / 4) - 1;
    } else {
        byte_indx = 0;
    }
    uint8_t bit_indx = indx % 4;
    leds_state[byte_indx] = leds_state[byte_indx] & ~(1 << bit_indx);
}

void cdc_init_led_signals(void) {
    leds_mu = xSemaphoreCreateMutex();
    assert(leds_mu != NULL);

    ESP_ERROR_CHECK(i2cdev_init());

    memset(&dev, 0, sizeof(i2c_dev_t));
    ESP_LOGI(TAG, "Initializing HT16K33 descriptor.");
    ESP_ERROR_CHECK(ht16k33_init_desc(&dev, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA,
                                      CONFIG_EXAMPLE_I2C_MASTER_SCL,
                                      HT16K33_DEFAULT_ADDR));

    ESP_LOGI(TAG, "Initializing HT16K33 device.");
    ESP_ERROR_CHECK(ht16k33_init(&dev));

    ESP_LOGI(TAG, "Turning display on.");
    ESP_ERROR_CHECK(ht16k33_display_setup(&dev, 1, HTK16K33_F_0HZ));

    ESP_LOGI(TAG, "OFF");
    ESP_ERROR_CHECK(ht16k33_ram_write(&dev, leds_state));
}

void cdc_enable_signal(uint8_t indx) {
    assert(indx < CONFIG_SD_CELLS_COUNT);
    assert(xSemaphoreTake(leds_mu, portMAX_DELAY) == pdTRUE);
    enable_signal_in_leds_state(indx);
    ESP_ERROR_CHECK(ht16k33_ram_write(&dev, leds_state));
    xSemaphoreGive(leds_mu);
}

void cdc_disable_signal(uint8_t indx) {
    assert(indx < CONFIG_SD_CELLS_COUNT);
    assert(xSemaphoreTake(leds_mu, portMAX_DELAY) == pdTRUE);
    disable_signal_in_leds_state(indx);
    ESP_ERROR_CHECK(ht16k33_ram_write(&dev, leds_state));
    xSemaphoreGive(leds_mu);
}

void cdc_deinit_led_signals(void) {
    ESP_LOGI(TAG, "Freeing device descriptor.");
    assert(xSemaphoreTake(leds_mu, portMAX_DELAY) == pdTRUE);
    ESP_ERROR_CHECK(ht16k33_free_desc(&dev));
    memset(&dev, 0, sizeof(i2c_dev_t));
    xSemaphoreGive(leds_mu);
}
#endif
