#ifndef BB_CONNECT_H
#define BB_CONNECT_H

#include "esp_err.h"

/**
 * @brief Inicializa el stack de red y NVS
 */
void bb_connect_init(void);

/**
 * @brief Ejecuta una actualización OTA desde una URL
 * @return esp_err_t ESP_OK si tiene éxito
 */
esp_err_t bb_connect_run_ota(const char *url);

#endif