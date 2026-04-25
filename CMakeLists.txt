cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(wireless_sensor_dashboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(wireless_sensor_dashboard
    main.c
)

target_link_libraries(wireless_sensor_dashboard
    pico_stdlib
    hardware_adc
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(wireless_sensor_dashboard 1)
pico_enable_stdio_uart(wireless_sensor_dashboard 0)

pico_add_extra_outputs(wireless_sensor_dashboard)