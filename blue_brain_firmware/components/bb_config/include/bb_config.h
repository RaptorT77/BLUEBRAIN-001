#ifndef BB_CONFIG_H
#define BB_CONFIG_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// =============================================================
// 🔧 DEFAULTS (Valores por defecto si no hay NVS)
// =============================================================
#define BB_DEFAULT_WIFI_SSID "Mega-2.4G-D9B1"
#define BB_DEFAULT_WIFI_PASS "AcP6U9qbZg"
#define BB_DEFAULT_SAMPLE_RATE 1000
#define BB_DEFAULT_N_SAMPLES 1024
#define BB_DEFAULT_MQTT_BROKER "mqtt://test.mosquitto.org"
#define BB_DEFAULT_MQTT_PORT 1883

// Fixed Compile-time Macros for DSP Buffers (must match max possible values)
#define BB_N_SAMPLES 2048
#define BB_SAMPLE_RATE_HZ 4000 // Max supported rate
#define BB_FFT_SIZE 2048

// =============================================================
// 📦 Estructura de Configuración del Sistema
// =============================================================
typedef struct {
  // WiFi
  char wifi_ssid[32];
  char wifi_pass[64];

  // MQTT
  char mqtt_broker_uri[64];
  int mqtt_port;
  char mqtt_user[32];
  char mqtt_pass[32];
  char mqtt_topic_telemetry[64];

  // Adquisición & DSP
  int sample_rate_hz;
  int n_samples; // 512, 1024, 2048

  // ESP-NOW
  bool espnow_enabled;
  uint8_t espnow_dest_mac[6];

  // Umbrales de Alarma
  float rms_alert_warn;  // G
  float rms_alert_crit;  // G
  float temp_alert_warn; // C
  float temp_alert_crit; // C

} bb_config_t;

// =============================================================
// ⚙️ API de Configuración
// =============================================================

/**
 * @brief Inicializa el sistema de configuración y carga desde NVS.
 * Si no existe, guarda los valores por defecto.
 */
esp_err_t bb_config_init(void);

/**
 * @brief Obtiene una copia de la configuración actual en RAM.
 */
const bb_config_t *bb_config_get(void);

/**
 * @brief Actualiza la configuración en RAM y la guarda en NVS.
 * @param new_config Nueva configuración a guardar.
 */
esp_err_t bb_config_set(const bb_config_t *new_config);

#endif // BB_CONFIG_H
