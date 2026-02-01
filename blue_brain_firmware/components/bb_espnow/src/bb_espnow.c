#include "bb_espnow.h"
#include "bb_config.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "BB_ESPNOW";

// Callback when data is sent
static void on_data_sent(const uint8_t *mac_addr,
                         esp_now_send_status_t status) {
  // Keep it quiet to avoid flooding logs on high frequency
  // ESP_LOGD(TAG, "Packet sent to "MACSTR", status: %d", MAC2STR(mac_addr),
  // status);
}

static esp_err_t add_peer(const uint8_t *mac_addr) {
  if (esp_now_is_peer_exist(mac_addr)) {
    return ESP_OK;
  }

  esp_now_peer_info_t peer = {0};
  memcpy(peer.peer_addr, mac_addr, 6);
  peer.channel = 0;         // Use current channel
  peer.ifidx = WIFI_IF_STA; // Send interface
  peer.encrypt = false;     // No encryption for open mesh/broadcast

  return esp_now_add_peer(&peer);
}

esp_err_t bb_espnow_init(void) {
  ESP_LOGI(TAG, "Initializing ESP-NOW...");

  esp_err_t ret = esp_now_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error initializing ESP-NOW: %s", esp_err_to_name(ret));
    return ret;
  }

  esp_now_register_send_cb((esp_now_send_cb_t)on_data_sent);

  // Add Broadcast Peer by default
  uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  add_peer(broadcast_mac);

  ESP_LOGI(TAG, "ESP-NOW Initialized (Broadcast Peer Added)");
  return ESP_OK;
}

esp_err_t bb_espnow_send(const bb_telemetry_t *data) {
  const bb_config_t *cfg = bb_config_get();

  if (!cfg->espnow_enabled) {
    return ESP_OK; // Disabled by user
  }

  // Determine target MAC
  const uint8_t *dest_mac = cfg->espnow_dest_mac;

  // Check if it's a valid unicast/multicast address different from Broadcast
  // For simplicity, if user set specific MAC, we try to ensure it's added.
  // If dest_mac is FF:FF... it is already added.

  // Note: cfg->espnow_dest_mac is 0xFF by default in my bb_config.c logic?
  // Let's check bb_config.c defaults.
  // Yes: memset(cfg->espnow_dest_mac, 0xFF, 6);

  // Ensure peer exists (idempotent)
  add_peer(dest_mac);

  // Send
  esp_err_t ret =
      esp_now_send(dest_mac, (uint8_t *)data, sizeof(bb_telemetry_t));
  if (ret != ESP_OK) {
    // ESP_LOGW(TAG, "Send Error: %s", esp_err_to_name(ret));
  }
  return ret;
}
