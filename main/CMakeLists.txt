set(SOURCES "blink.c" "soil_sensor.c" "main.c")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ".")
