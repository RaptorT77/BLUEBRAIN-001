#include "bb_config.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "BB_CONFIG";
static const char *NVS_NAMESPACE = "bb_cfg";
static const char *NVS_KEY = "system";

static bb_config_t g_config;

static void load_defaults(bb_config_t *cfg) {
  // WiFi
  strlcpy(cfg->wifi_ssid, BB_DEFAULT_WIFI_SSID, sizeof(cfg->wifi_ssid));
  strlcpy(cfg->wifi_pass, BB_DEFAULT_WIFI_PASS, sizeof(cfg->wifi_pass));

  // MQTT
  strlcpy(cfg->mqtt_broker_uri, BB_DEFAULT_MQTT_BROKER,
          sizeof(cfg->mqtt_broker_uri));
  cfg->mqtt_port = BB_DEFAULT_MQTT_PORT;
  strlcpy(cfg->mqtt_user, "", sizeof(cfg->mqtt_user));
  strlcpy(cfg->mqtt_pass, "", sizeof(cfg->mqtt_pass));
  strlcpy(cfg->mqtt_topic_telemetry, "bluebrain/telemetry",
          sizeof(cfg->mqtt_topic_telemetry));

  // Acquisition
  cfg->sample_rate_hz = BB_DEFAULT_SAMPLE_RATE;
  cfg->n_samples = BB_DEFAULT_N_SAMPLES;

  // ESP-NOW (Default Disabled)
  cfg->espnow_enabled = false;
  memset(cfg->espnow_dest_mac, 0xFF, 6); // Broadcast by default

  // Umbrales
  cfg->rms_alert_warn = 2.0f;
  cfg->rms_alert_crit = 4.0f;
  cfg->temp_alert_warn = 60.0f;
  cfg->temp_alert_crit = 80.0f;
}

esp_err_t bb_config_init(void) {
  esp_err_t err;
  nvs_handle_t my_handle;

  ESP_LOGI(TAG, "Initializing Configuration...");

  // Open
  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
    return err;
  }

  // Read
  size_t required_size = sizeof(bb_config_t);
  err = nvs_get_blob(my_handle, NVS_KEY, &g_config, &required_size);

  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Config Loaded from NVS");

    // Migration check: If new fields are 0, populate them
    if (g_config.rms_alert_warn == 0.0f)
      g_config.rms_alert_warn = 2.0f;
    if (g_config.rms_alert_crit == 0.0f)
      g_config.rms_alert_crit = 4.0f;
    if (g_config.temp_alert_warn == 0.0f)
      g_config.temp_alert_warn = 60.0f;
    if (g_config.temp_alert_crit == 0.0f)
      g_config.temp_alert_crit = 80.0f;

  } else if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGW(TAG, "Config not found in NVS. Loading defaults.");
    load_defaults(&g_config);

    // Save defaults
    err = nvs_set_blob(my_handle, NVS_KEY, &g_config, sizeof(bb_config_t));
    if (err == ESP_OK) {
      nvs_commit(my_handle);
      ESP_LOGI(TAG, "Defaults saved to NVS");
    } else {
      ESP_LOGE(TAG, "Error saving defaults: %s", esp_err_to_name(err));
    }
  } else {
    ESP_LOGE(TAG, "Error reading config: %s", esp_err_to_name(err));
  }

  // Close
  nvs_close(my_handle);
  return err;
}

const bb_config_t *bb_config_get(void) { return &g_config; }

esp_err_t bb_config_set(const bb_config_t *new_config) {
  if (new_config == NULL)
    return ESP_ERR_INVALID_ARG;

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
    return err;

  // Update RAM
  memcpy(&g_config, new_config, sizeof(bb_config_t));

  // Update NVS
  err = nvs_set_blob(my_handle, NVS_KEY, &g_config, sizeof(bb_config_t));
  if (err == ESP_OK) {
    err = nvs_commit(my_handle);
    ESP_LOGI(TAG, "Config saved successfully");
  } else {
    ESP_LOGE(TAG, "Error saving config: %s", esp_err_to_name(err));
  }

  nvs_close(my_handle);
  return err;
}
