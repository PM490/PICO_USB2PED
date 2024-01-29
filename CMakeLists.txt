# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build fuctions from Pico SDK
include ($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(PICO_USB2PED C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Previous commands added manually.

add_executable(${PROJECT_NAME}
        main.cpp
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# pull in common dependencies
target_link_libraries(${PROJECT_NAME} pico_stdlib)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# add url via pico_set_program_url
# example_auto_set_url(${PROJECT_NAME})
