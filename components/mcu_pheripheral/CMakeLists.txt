idf_component_register(SRCS "spi.c" "spiffs.c" "tim.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver spiffs
                    PRIV_REQUIRES driver spiffs)

