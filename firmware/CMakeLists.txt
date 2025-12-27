# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(pico_mouse_joystick C CXX ASM)
pico_sdk_init()

add_executable(pico_mouse_joystick 
    main.c
    usb_descriptors.c
)

target_include_directories(pico_mouse_joystick PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

pico_set_program_name(pico_mouse_joystick "Pico Mouse RGB Joystick")
pico_set_program_version(pico_mouse_joystick "2.0")

pico_enable_stdio_usb(pico_mouse_joystick 0)
pico_enable_stdio_uart(pico_mouse_joystick 0)

target_link_libraries(pico_mouse_joystick
    pico_stdlib
    pico_cyw43_arch_none
    hardware_adc
    hardware_gpio
    hardware_pwm
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(pico_mouse_joystick)