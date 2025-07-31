#include "battery_controller.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define BATTERY_ADC_UNIT     ADC_UNIT_1
#define BATTERY_ADC_CHANNEL  ADC_CHANNEL_3  // GPIO3
#define BATTERY_DIVIDER_RATIO 1.3f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;
static bool do_calibration = false;

void battery_monitor_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,  // заменить DB_11 на DB_12 (они эквивалентны)
    };
    adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_config);

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        do_calibration = true;
    }
}

float battery_monitor_read_voltage(void) {
    int raw = 0;
    adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &raw);

    int voltage_mv = 0;
    if (do_calibration) {
        adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv);
    } else {
        voltage_mv = (raw * 3300) / 4095;  // fallback
    }

    float vbat = (voltage_mv / 1000.0f) * BATTERY_DIVIDER_RATIO;
    return vbat;
}
