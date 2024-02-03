set(requires "")
if(${IDF_TARGET} STREQUAL "linux")
    list(APPEND requires esp_stubs esp-tls esp_http_server protocol_examples_common nvs_flash)
endif()
idf_component_register(SRCS "btdrive.c" "bt_main.c" "uart.c" "sio.c" "console.c"
		INCLUDE_DIRS ".")
