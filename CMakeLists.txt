cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)


project(HomeControl C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT_NAME}
                HomeControl.c 
            )

pico_set_program_name(${PROJECT_NAME} "HomeControl")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_gpio
        hardware_adc
        pico_cyw43_arch_lwip_threadsafe_background
        FreeRTOS-Kernel         # Kernel do FreeRTOS
        FreeRTOS-Kernel-Heap4   # Gerenciador de memoria
)

# Add the standard include files to the build
target_include_directories(HomeControl PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/lwip/src/include
    ${PICO_SDK_PATH}/lib/lwip/src/include/arch
    ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
)

target_sources(HomeControl PRIVATE
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
)

# Add any user requested libraries
pico_add_extra_outputs(${PROJECT_NAME})
