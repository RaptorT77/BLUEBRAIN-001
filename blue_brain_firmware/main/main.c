#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "hardware_defs.h"

// Component headers will be included here as they are created
// #include "bb_power.h"
// #include "bb_connect.h"
// ...

static const char *TAG = "MAIN";

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Blue Brain Firmware Started");
    ESP_LOGI(TAG, "Hardware: XIAO ESP32-S3");
    
    // TODO: Initialize components
    // bb_power_init();
    // bb_storage_init();
    // ...

    // Main Loop / Orchestrator
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
