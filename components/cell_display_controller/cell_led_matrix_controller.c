#include "sdkconfig.h"
#ifdef CONFIG_CDC_LEDS_MATRIX
#include "cell_led_controller.h"
#include <esp_log.h>
#include <ht16k33.h>
#include <i2cdev.h>
#include <string.h>

static char *TAG = "leds-matrix-controller";

// TODO TODO
// decorate with mutex
static i2c_dev_t dev;

static uint8_t leds_state[HT16K33_RAM_SIZE_BYTES] = {0};

void cdc_init_led_signals(void) {

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

void cdc_enable_signal(uint8_t indx);
void cdc_disable_signal(uint8_t indx);

void cdc_deinit_led_signals(void) {
    ESP_LOGI(TAG, "Freeing device descriptor.");
    ESP_ERROR_CHECK(ht16k33_free_desc(&dev));
    memset(&dev, 0, sizeof(i2c_dev_t));
}
#endif
