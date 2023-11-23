idf_component_register(SRCS "ble.c" "ble_adv.c" "ble_gap.c" "ble_gatts.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt nvs_flash watch)

