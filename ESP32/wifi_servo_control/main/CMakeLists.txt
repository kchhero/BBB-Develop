idf_component_register(SRCS "wifi_servo_control.c"
                    INCLUDE_DIRS ".")

set(web_files "web/index.html")
target_add_binary_data(${COMPONENT_TARGET} "${web_files}" TEXT)