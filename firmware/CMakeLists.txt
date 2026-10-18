set(BOARD "nucleo" CACHE STRING "Target board: nucleo or blackpill")
set_property(CACHE BOARD PROPERTY STRINGS nucleo blackpill)

if(BOARD STREQUAL "nucleo")
    set(BOARD_DEFINE BOARD_NUCLEO=1)
elseif(BOARD STREQUAL "blackpill")
    set(BOARD_DEFINE BOARD_BLACKPILL=1)
else()
    message(FATAL_ERROR "Unknown BOARD=${BOARD} (expected nucleo or blackpill)")
endif()

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_library(freertos_kernel STATIC
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/tasks.c
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/queue.c
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/list.c
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/timers.c
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/MemMang/heap_4.c
)

target_include_directories(freertos_kernel PUBLIC
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/include
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(freertos_kernel PRIVATE ${CPU_FLAGS} -ffunction-sections -fdata-sections)
target_compile_definitions(freertos_kernel PUBLIC ${BOARD_DEFINE})

add_executable(snmp_f411.elf
    src/startup_stm32f411xe.c
    src/system_stm32f4xx.c
    src/bsp.c
    src/app_tasks.c
    src/syscalls.c
    src/main.c
)

target_include_directories(snmp_f411.elf PRIVATE
    include
    ${PROJECT_SOURCE_DIR}/third_party/CMSIS/Core/Include
    ${PROJECT_SOURCE_DIR}/third_party/CMSIS/Device/ST/STM32F4xx/Include
)

target_compile_definitions(snmp_f411.elf PRIVATE
    STM32F411xE
    ${BOARD_DEFINE}
)

target_compile_options(snmp_f411.elf PRIVATE
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -fno-builtin
)

target_link_libraries(snmp_f411.elf PRIVATE snmp freertos_kernel)

target_link_options(snmp_f411.elf PRIVATE
    ${CPU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F411xE.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.map
    --specs=nano.specs
    -Wl,--start-group
    -lc
    -lm
    -Wl,--end-group
)

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy)
find_program(CMAKE_SIZE arm-none-eabi-size)

add_custom_command(TARGET snmp_f411.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:snmp_f411.elf> ${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:snmp_f411.elf> ${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:snmp_f411.elf>
    COMMENT "Generate BIN/HEX and print size"
)
