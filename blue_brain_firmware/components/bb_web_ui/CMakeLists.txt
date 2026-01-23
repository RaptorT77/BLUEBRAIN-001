idf_component_register(SRCS "src/bb_web_ui.c"
                    INCLUDE_DIRS "include" // Optional if we add headers later
                    REQUIRES esp_http_server lwip)
