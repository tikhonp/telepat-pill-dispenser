#include "sdkconfig.h"
#include <assert.h>
#include <strings.h>
#ifdef CONFIG_CDC_LEDS_MATRIX
#include "cell_led_controller.h"
#include <esp_log.h>
#include <ht16k33.h>
#include <i2cdev.h>
#include <string.h>

/*
* So if  matrix is 4x7 where 4-`a`s and 7 is `com`
*
* It whould look like this
*
*      com0 com1 com2 com3 com4 com5 com6
* row0 -    -    -    -    -    -    -    byte0 [0, 0, 0, 0, 0, 0, 0, unused] system idxs 0-6
* row1 -    -    -    -    -    -    -    byte1 [0, 0, 0, 0, 0, 0, 0, unused]             7-13
* row2 -    -    -    -    -    -    -    byte2 [0, 0, 0, 0, 0, 0, 0, unused]             14-20
* row3 -    -    -    -    -    -    -    byte3 [0, 0, 0, 0, 0, 0, 0, unused]             21-27
*
* There is byte array `leds_state`
*
* And if i want set let's say (2, 4) led (row1, com3) i need to set fourth bit of byte1
* 
* So i count system cell indexes from left to right and from top to bottom
*/

static char *TAG = "leds-matrix-controller";

static i2c_dev_t dev;
static uint8_t leds_state[HT16K33_RAM_SIZE_BYTES] = {0};

static SemaphoreHandle_t leds_mu;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
    ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'),                  \
        ((byte) & 0x20 ? '1' : '0'), ((byte) & 0x10 ? '1' : '0'),              \
        ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),              \
        ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

static void print_leds_state() {
    for (int i = 0; i < HT16K33_RAM_SIZE_BYTES; i++)
        printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(leds_state[i]));
}

static void enable_signal_in_leds_state(uint8_t indx) {
    uint8_t byte_indx = indx / 7;
    uint8_t bit_indx = indx % 7;
    leds_state[byte_indx] = leds_state[byte_indx] | (1 << bit_indx);
    printf("Enabling %d\n", indx);
    print_leds_state();
}

static void disable_signal_in_leds_state(uint8_t indx) {
    uint8_t byte_indx = indx / 7;
    uint8_t bit_indx = indx % 7;
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
