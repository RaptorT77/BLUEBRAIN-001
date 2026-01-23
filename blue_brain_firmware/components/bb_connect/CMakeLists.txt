idf_component_register(SRCS "src/bb_connect.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_wifi nvs_flash esp_http_client esp_https_ota mbedtls)