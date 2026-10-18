set(BOARD "nucleo" CACHE STRING "Target board: nucleo or blackpill") # плата по умолчанию — Nucleo
set_property(CACHE BOARD PROPERTY STRINGS nucleo blackpill) # допустимые значения BOARD

if(BOARD STREQUAL "nucleo") # сборка под NUCLEO-F411RE
    set(BOARD_DEFINE BOARD_NUCLEO=1) # макрос платы Nucleo
elseif(BOARD STREQUAL "blackpill") # сборка под Black Pill
    set(BOARD_DEFINE BOARD_BLACKPILL=1) # макрос платы Black Pill
else() # неизвестное имя платы
    message(FATAL_ERROR "Unknown BOARD=${BOARD} (expected nucleo or blackpill)") # остановить CMake
endif() # конец выбора платы

set(CPU_FLAGS # флаги Cortex-M4F
    -mcpu=cortex-m4 # ядро STM32F411
    -mthumb # набор Thumb
    -mfpu=fpv4-sp-d16 # FPU
    -mfloat-abi=hard # аппаратный float
) # конец CPU_FLAGS

add_library(freertos_kernel STATIC # ядро FreeRTOS отдельной библиотекой
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/tasks.c # планировщик
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/queue.c # очереди
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/list.c # списки
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/timers.c # программные таймеры
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c # порт Cortex-M4F
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/MemMang/heap_4.c # куча heap_4
) # конец исходников ядра

target_include_directories(freertos_kernel PUBLIC # заголовки ядра
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/include # API FreeRTOS
    ${PROJECT_SOURCE_DIR}/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM4F # portmacro.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include # FreeRTOSConfig.h
) # конец include ядра

target_compile_options(freertos_kernel PRIVATE ${CPU_FLAGS} -ffunction-sections -fdata-sections) # те же флаги CPU
target_compile_definitions(freertos_kernel PUBLIC ${BOARD_DEFINE}) # макрос платы виден ядру

add_executable(snmp_f411.elf # образ прошивки
    src/startup_stm32f411xe.c # таблица векторов и Reset_Handler
    src/system_stm32f4xx.c # тактирование 100 МГц
    src/bsp.c # GPIO, UART, очередь кадров
    src/app_tasks.c # задачи led и snmp
    src/syscalls.c # заглушки newlib
    src/main.c # вход в приложение
) # конец исходников прошивки

target_include_directories(snmp_f411.elf PRIVATE # заголовки прошивки
    include # board.h, bsp.h, FreeRTOSConfig.h
    ${PROJECT_SOURCE_DIR}/third_party/CMSIS/Core/Include # core_cm4.h
    ${PROJECT_SOURCE_DIR}/third_party/CMSIS/Device/ST/STM32F4xx/Include # stm32f411xe.h
) # конец include прошивки

target_compile_definitions(snmp_f411.elf PRIVATE # макросы компиляции
    STM32F411xE # выбрать CMSIS-устройство F411xE
    ${BOARD_DEFINE} # nucleo или blackpill
) # конец definitions

target_compile_options(snmp_f411.elf PRIVATE # флаги компиляции прошивки
    ${CPU_FLAGS} # Cortex-M4F
    -Wall # основные предупреждения
    -Wextra # дополнительные предупреждения
    -ffunction-sections # секция на функцию
    -fdata-sections # секция на данные
    -fno-builtin # не подменять функции встроенными
) # конец compile options

target_link_libraries(snmp_f411.elf PRIVATE snmp freertos_kernel) # агент и ядро RTOS

target_link_options(snmp_f411.elf PRIVATE # флаги линковки
    ${CPU_FLAGS} # ABI должен совпасть с объектами
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F411xE.ld # скрипт памяти
    -Wl,--gc-sections # выбросить неиспользуемые секции
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.map # карта линковки
    --specs=nano.specs # newlib-nano
    -Wl,--start-group # разрешить циклические ссылки libc
    -lc # стандартная библиотека C
    -lm # математика
    -Wl,--end-group # конец группы
) # конец link options

find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy) # найти objcopy
find_program(CMAKE_SIZE arm-none-eabi-size) # найти size

add_custom_command(TARGET snmp_f411.elf POST_BUILD # после линковки
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:snmp_f411.elf> ${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.bin # сырой BIN
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:snmp_f411.elf> ${CMAKE_CURRENT_BINARY_DIR}/snmp_f411.hex # Intel HEX
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:snmp_f411.elf> # напечатать text/data/bss
    COMMENT "Собрать BIN/HEX и напечатать размер" # подпись команды
) # конец POST_BUILD
