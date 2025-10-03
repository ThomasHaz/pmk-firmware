cmake_minimum_required(VERSION 3.13)
include(../pico_sdk_import.cmake)
project(keyboard_dongle C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico2_w)
pico_sdk_init()

add_executable(keyboard_dongle
    main.c
    usb_descriptors.c
    usb_hid.c
    ../lib/utils/timer.c
    ../lib/features/layers.c
    ../lib/features/modtap.c
    ../lib/features/oneshot.c
    ../lib/features/combos.c
    ../lib/features/tapdance.c
    ../lib/features/mouse.c
    ../keymaps/default/keymap.c
)

target_include_directories(keyboard_dongle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/features
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/../keymaps/default
)

target_compile_definitions(keyboard_dongle PRIVATE
    PICO_RP2350=1
    CYW43_LWIP=1
    PICO_CYW43_ARCH_POLL=1
)

target_link_libraries(keyboard_dongle
    pico_stdlib
    pico_unique_id
    pico_cyw43_arch_lwip_poll
    tinyusb_device
    tinyusb_board
    hardware_timer
    hardware_gpio
)

pico_enable_stdio_usb(keyboard_dongle 0)
pico_enable_stdio_uart(keyboard_dongle 1)
pico_add_extra_outputs(keyboard_dongle)