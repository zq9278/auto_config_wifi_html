idf_component_register(SRCS  "webserver.c"
        SRCS "../connectwifi/ConnectWIFI.c"
        SRCS "../change_html/Change_HTML.c"
                    INCLUDE_DIRS "."
        INCLUDE_DIRS "../connectwifi"
        INCLUDE_DIRS "../change_html"
                    EMBED_FILES "index.html"
                    REQUIRES esp_wifi esp_http_server nvs_flash)
