idf_component_register(
                    SRCS "ConnectWIFI.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_event esp_wifi nvs_flash esp_http_server
                   )
