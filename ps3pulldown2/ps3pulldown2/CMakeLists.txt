cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include ($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ps3pulldown2 CXX C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(ps3pulldown2
    ps3pulldown2.c usb_device.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(ps3pulldown2 pico_stdlib hardware_resets hardware_irq pico_multicore)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(ps3pulldown2)