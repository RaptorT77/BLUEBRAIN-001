#ifndef BB_ESPNOW_H
#define BB_ESPNOW_H

#include "bb_connect.h" // For bb_telemetry_t
#include "esp_err.h"


/**
 * @brief Initialize ESP-NOW.
 * Requires WiFi to be initialized (station or softap mode).
 */
esp_err_t bb_espnow_init(void);

/**
 * @brief Send telemetry data via ESP-NOW.
 * Uses broadcast or the configured destination MAC from bb_config.
 * @param data Pointer to telemetry structure.
 */
esp_err_t bb_espnow_send(const bb_telemetry_t *data);

#endif // BB_ESPNOW_H
