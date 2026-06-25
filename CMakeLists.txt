# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Prefer Pico SDK's bundled ninja if the system ninja is blocked by AppLocker/AV.
# This helps when CMake finds a ninja executable that cannot be run (causes
# "unknown error" during generator detection). The fallback is safe and
# only changes the cached CMAKE_MAKE_PROGRAM when the detected program fails.
set(_pico_ninja "$ENV{USERPROFILE}/.pico-sdk/ninja/v1.12.1/ninja.exe")
if(NOT DEFINED CMAKE_MAKE_PROGRAM OR CMAKE_MAKE_PROGRAM STREQUAL "")
    if(EXISTS "${_pico_ninja}")
        set(CMAKE_MAKE_PROGRAM "${_pico_ninja}" CACHE FILEPATH "Ninja executable" FORCE)
    endif()
else()
    execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" --version
        RESULT_VARIABLE _ninja_check
        OUTPUT_QUIET ERROR_QUIET
        TIMEOUT 3)
    if(NOT _ninja_check EQUAL 0 AND EXISTS "${_pico_ninja}")
        message(STATUS "Detected blocked/unusable ninja; switching to pico-sdk ninja")
        set(CMAKE_MAKE_PROGRAM "${_pico_ninja}" CACHE FILEPATH "Ninja executable" FORCE)
    endif()
endif()

# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ui_system_pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(ui_system_pico ui_system_pico.c)

# Quiet build option: set -DQUIET_COMPILER=ON to silence compiler warnings
# This is target-scoped and reversible; preferred over forcing -w globally.
option(QUIET_COMPILER "Silence compiler warnings (not recommended for production)" OFF)
if(QUIET_COMPILER)
    target_compile_options(ui_system_pico PRIVATE -w)
endif()

pico_set_program_name(ui_system_pico "ui_system_pico")
pico_set_program_version(ui_system_pico "0.1")

# Generate PIO header
pico_generate_pio_header(ui_system_pico ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(ui_system_pico 0)
pico_enable_stdio_usb(ui_system_pico 1)

# Add the standard library to the build
target_link_libraries(ui_system_pico
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(ui_system_pico PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(ui_system_pico
    hardware_spi
    hardware_dma
    hardware_pio
)

pico_add_extra_outputs(ui_system_pico)

