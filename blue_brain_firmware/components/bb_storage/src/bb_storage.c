#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "BB_STORAGE";

void bb_storage_init(void)
{
    ESP_LOGI(TAG, "Initializing Storage (SPIFFS/NVS)");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 5,
      .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS mounted");
    }
}
