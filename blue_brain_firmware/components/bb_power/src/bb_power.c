#include "bb_power.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "hardware_defs.h"


static const char *TAG = "BB_POWER";

static adc_oneshot_unit_handle_t adc1_handle;

void bb_power_init(void) {
  ESP_LOGI(TAG, "Initializing Power Management...");

  // ADC Init
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1, // GPIO 7 is ADC1_CH6 on ESP32-S3
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

  // ADC Config
  adc_oneshot_chan_cfg_t config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12, // 11dB or 12dB for <3.3V (Using new enum name
                                // if available, else 11)
      // Note: ESP-IDF 5.x uses ADC_ATTEN_DB_12 (approx 11dB deprecated name in
      // some versions)
  };

  // Assuming PIN_BAT_ADC maps to a known channel.
  // GPIO 7 -> ADC1 Channel 6
  // We should ideally determine channel from IO, but for now hardcoding based
  // on S3.
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));

  ESP_LOGI(TAG, "Power Management Initialized");
}

void bb_power_enter_deep_sleep(uint64_t sleep_time_us) {
  ESP_LOGI(TAG, "Entering Deep Sleep for %llu us", sleep_time_us);

  // Enable Wakeup Timer
  esp_sleep_enable_timer_wakeup(sleep_time_us);

  // Enter Sleep
  esp_deep_sleep_start();
}

float bb_power_get_battery_voltage(void) {
  int adc_raw;
  esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "ADC Read failed");
    return 0.0f;
  }

  // Simple conversion (Raw -> Voltage)
  // 3.3V ref, 12 bit (4096)
  // V = Raw * 3.3 / 4095
  // Apply voltage divider factor (e.g. 2x if 100k/100k)
  // Assuming 2x divider for LiPo (4.2V max input to 3.3V pin requires divider)
  float voltage = (adc_raw * 3.3f / 4095.0f) * 2.0f;

  return voltage;
}
