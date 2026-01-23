idf_component_register(SRCS "src/bb_storage.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver nvs_flash spiffs)