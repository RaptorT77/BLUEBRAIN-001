/*
 * Práctica 2: Escáner Wi-Fi con ESP32-S3
 *
 * Objetivos:
 * 1. Inicializar la NVS (Sistema de Archivos para configuración).
 * 2. Inicializar el Stack Wi-Fi en modo Estación (STA).
 * 3. Escanear redes cercanas y mostrar su SSID (Nombre) y RSSI (Potencia).
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_SCAN";

// Función auxiliar para imprimir el tipo de autenticación
const char* get_auth_mode_name(wifi_auth_mode_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
        default: return "UNKNOWN";
    }
}

void wifi_scan_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Iniciando escaneo Wi-Fi...");

        // Configuración del escaneo
        wifi_scan_config_t scan_config = {
            .ssid = 0,
            .bssid = 0,
            .channel = 0,
            .show_hidden = true
        };

        // Inicia el escaneo y bloquea hasta que termine (true al final)
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

        ESP_LOGI(TAG, "Escaneo completado.");

        // Obtener la lista de APs encontrados
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));

        ESP_LOGI(TAG, "REDES ENCONTRADAS: %d", ap_count);
        printf("----------------------------------------------------------------\n");
        printf("%-32s | %-15s | %s\n", "SSID", "AUTH", "RSSI");
        printf("----------------------------------------------------------------\n");

        for (int i = 0; i < ap_count; i++) {
            printf("%-32s | %-15s | %d\n", 
                   ap_list[i].ssid, 
                   get_auth_mode_name(ap_list[i].authmode), 
                   ap_list[i].rssi);
        }
        printf("----------------------------------------------------------------\n");

        // Liberar memoria
        free(ap_list);
        
        // Esperar 10 segundos antes del siguiente escaneo
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // 1. Inicializar NVS (Requisito para Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicializar Netif y Loop de eventos
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 3. Configurar Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 4. Crear tarea de escaneo
    xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL);
}