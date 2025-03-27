#include "button_controller.h"
#include "button_gpio.h"
#include "button_types.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "iot_button.h"

#define BUTTON_PIN 6

/*static const char *TAG = "button-controller";*/

static EventGroupHandle_t bc_event_group;
const int bc_single_press = BIT0;

static void button_event_cb(void *arg, void *data) {
    xEventGroupSetBits(bc_event_group, bc_single_press);
}

void bc_wait_for_single_press(void) {
    while (true) {
        if (xEventGroupWaitBits(bc_event_group, bc_single_press, true, true,
                                portMAX_DELAY) == pdTRUE) {
            return;
        }
    }
}

void bc_init(void) {
    bc_event_group = xEventGroupCreate();

    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON_PIN,
        .active_level = BUTTON_INACTIVE,
        .enable_power_save = false,
    };

    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);

    ret = iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_cb,
                                 NULL);

    ESP_ERROR_CHECK(ret);
}
