#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h" // Necesario para la validación SSL
#include "bb_connect.h"

static const char *TAG = "BB_CONNECT";

/**
 * @brief Función para ejecutar la actualización OTA (Over-The-Air)
 */
esp_err_t bb_connect_run_ota(const char *url) {
    if (url == NULL) {
        ESP_LOGE(TAG, "URL de OTA no válida");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Iniciando proceso OTA desde: %s", url);

    // 1. Configuración del cliente HTTP (Capa de transporte)
    esp_http_client_config_t http_config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach, // Adjunta certificados raíz de Espressif
        .keep_alive_enable = true,
    };

    // 2. Configuración específica para el sistema HTTPS OTA
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    // 3. Ejecutar la actualización
    // Esta función bloquea hasta que termina o falla
    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Actualización completada con éxito. Reiniciando el sistema...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Fallo en la actualización OTA: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Inicialización básica de Wi-Fi para el sistema Blue Brain
 */
void bb_connect_init(void) {
    ESP_LOGI(TAG, "Inicializando stack de red...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}