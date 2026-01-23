#include "bb_power.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "BB_POWER";

void bb_power_init(void)
{
    ESP_LOGI(TAG, "Power Management Initialized");
}

void bb_power_enter_deep_sleep(uint64_t sleep_time_us)
{
    ESP_LOGI(TAG, "Entering Deep Sleep for %llu us", sleep_time_us);
    esp_deep_sleep(sleep_time_us);
}

float bb_power_get_battery_voltage(void)
{
    // Implementation needed
    return 3.7f; // Dummy value
}
