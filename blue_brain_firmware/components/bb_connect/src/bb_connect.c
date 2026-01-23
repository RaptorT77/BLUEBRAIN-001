#include "bb_connect.h"
#include "esp_log.h"
#include "esp_https_ota.h"

static const char *TAG = "BB_CONNECT";

void bb_connect_init(void)
{
    ESP_LOGI(TAG, "Connectivity Module Initialized");
}

void bb_connect_wifi_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi Station Mode");
    // TODO: WiFi Init Logic
}

void bb_connect_check_update(void)
{
    ESP_LOGI(TAG, "Checking for OTA Updates...");
    // TODO: Check server for version
}

void bb_connect_run_ota(const char *url)
{
    ESP_LOGI(TAG, "Starting OTA from: %s", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Success, restarting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA Failed");
    }
}
