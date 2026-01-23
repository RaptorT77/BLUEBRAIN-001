idf_component_register(SRCS "src/bb_connect.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_https_ota esp_wifi nvs_flash app_update esp_event)
