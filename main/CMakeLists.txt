idf_component_register(SRCS "main.c"
                    SRCS "../components/dns_server/dns_server.c"
                    SRCS "../components/webserver/webserver.c"
                    SRCS "../components/connectwifi/ConnectWIFI.c"
                    #SRCS "../components/change_html/Change_HTML.c"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../components/dns_server"
                    INCLUDE_DIRS "../components/webserver"
                    INCLUDE_DIRS "../components/connectwifi"
                    #INCLUDE_DIRS "../components/change_html"
                    )

