cmake_minimum_required(VERSION 3.13)


include(../pico_sdk_import.cmake)

project(keyboard_right C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico2_w)

pico_sdk_init()

add_executable(keyboard_right
    main.c
    ../lib/matrix/matrix.c
    ../lib/utils/timer.c
)

target_include_directories(keyboard_right PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/matrix
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/utils
)

target_compile_definitions(keyboard_right PRIVATE
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    CYW43_LWIP=1
    PICO_CYW43_ARCH_POLL=1
    PICO_RP2350=1
)

target_link_libraries(keyboard_right
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_gpio
    hardware_timer
)

pico_enable_stdio_usb(keyboard_right 0)
pico_enable_stdio_uart(keyboard_right 1)

pico_add_extra_outputs(keyboard_right)