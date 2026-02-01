#include "bb_connect.h"
#include "bb_power.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <string.h>

#include "bb_config.h"

static const char *TAG = "bb_connect";
static esp_mqtt_client_handle_t mqtt_client = NULL;
QueueHandle_t xQueueTelemetry = NULL;

// --- WiFi Configuration (Loaded from bb_config.h) ---
#define WIFI_SSID BB_WIFI_SSID
#define WIFI_PASS BB_WIFI_PASS
#define MQTT_BROKER CONFIG_BB_MQTT_URI

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGW(TAG, "WiFi desconectado, reintentando...");
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGI(TAG, "WiFi conectado, iniciando MQTT...");
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT Conectado");
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "MQTT Desconectado");
    break;
  default:
    break;
  }
}

#include "bb_espnow.h"

void Task_Comms(void *pvParameters) {
  bb_telemetry_t data;
  char json_payload[256]; // Increased buffer size

  while (1) {
    if (xQueueReceive(xQueueTelemetry, &data, portMAX_DELAY)) {

      // Update Battery Voltage
      data.batt_v = bb_power_get_battery_voltage();

      // Send via ESP-NOW
      bb_espnow_send(&data);

      // JSON for MQTT
      snprintf(
          json_payload, sizeof(json_payload),
          "{\"rms\":%.3f,\"peak\":%.3f,\"p2p\":%.3f,\"crest\":%.2f,"
          "\"temp\":%.2f,\"dom_freq\":%.1f,\"band_lo\":%.3f,\"band_hi\":%.3f,"
          "\"ai_class\":%d,\"ai_conf\":%.2f,\"batt\":%.2f}",
          data.vib_rms, data.vib_peak, data.vib_p2p, data.crest_factor,
          data.temp_c, data.vib_dom_freq, data.vib_band_low, data.vib_band_high,
          data.ai_class, data.ai_conf, data.batt_v);

      if (mqtt_client != NULL) {
        esp_mqtt_client_publish(mqtt_client, "hcaa/plcs/bluebrain/telemetry",
                                json_payload, 0, 1, 0);
        ESP_LOGI(TAG, "Tel: %s", json_payload);
      } else {
        ESP_LOGW(TAG, "No MQTT: %s", json_payload);
      }
    }
  }
}

void bb_connect_init(void) {
  // 1. Create Queue
  xQueueTelemetry = xQueueCreate(10, sizeof(bb_telemetry_t));
  ESP_LOGI(TAG, "Queue Created");

  // 1.5 Init Power
  bb_power_init();

  const bb_config_t *sys_cfg = bb_config_get();

  // 2. Init WiFi
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  // Configure STA
  wifi_config_t wifi_con = {0};
  strlcpy((char *)wifi_con.sta.ssid, sys_cfg->wifi_ssid,
          sizeof(wifi_con.sta.ssid));
  strlcpy((char *)wifi_con.sta.password, sys_cfg->wifi_pass,
          sizeof(wifi_con.sta.password));

  ESP_LOGI(TAG, "WiFi STA: %s", wifi_con.sta.ssid);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_con));

  // Configure AP
  wifi_config_t ap_config = {
      .ap = {.ssid = "BlueBrain_Setup",
             .password = "",
             .channel = 1,
             .max_connection = 4,
             .authmode = WIFI_AUTH_OPEN},
  };
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

  ESP_ERROR_CHECK(esp_wifi_start());

  // 2.5 Init ESP-NOW (Must be after WiFi Start)
  bb_espnow_init();

  // 3. Init MQTT
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = sys_cfg->mqtt_broker_uri,
      .broker.address.port = sys_cfg->mqtt_port,
      .credentials.username = sys_cfg->mqtt_user,
      .credentials.authentication.password = sys_cfg->mqtt_pass,
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
  esp_mqtt_client_start(mqtt_client);

  // 4. Launch Task
  xTaskCreatePinnedToCore(Task_Comms, "Task_Comms", 4096, NULL, 3, NULL, 0);
}