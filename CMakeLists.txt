cmake_minimum_required(VERSION 3.12)

include (pico_sdk_import.cmake)

project(npdrv C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_executable(npdrv npdrv.c)

pico_generate_pio_header(npdrv ${CMAKE_CURRENT_LIST_DIR}/neopixel.pio)

target_link_libraries(npdrv pico_stdlib hardware_pio hardware_dma)

pico_enable_stdio_usb(npdrv 1)
pico_enable_stdio_uart(npdrv 0)

pico_add_extra_outputs(npdrv)
