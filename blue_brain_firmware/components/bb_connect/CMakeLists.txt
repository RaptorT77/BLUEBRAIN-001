idf_component_register(SRCS "src/bb_connect.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event mqtt esp_netif nvs_flash esp_http_client esp_https_ota mbedtls
                    PRIV_REQUIRES bb_power bb_config bb_espnow)