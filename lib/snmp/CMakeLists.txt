add_library(snmp STATIC # статическая библиотека агента
    src/snmp.c # BER и обработка PDU
    src/frame.c # кадры A5 5A
) # конец списка исходников

target_include_directories(snmp PUBLIC include) # публичные заголовки lib/snmp/include
target_compile_options(snmp PRIVATE -Wall -Wextra -Wpedantic) # строгие предупреждения

if(CMAKE_SYSTEM_NAME STREQUAL "Generic") # сборка под Cortex-M4
    target_compile_options(snmp PRIVATE # флаги ядра и FPU
        -mcpu=cortex-m4 # процессор STM32F411
        -mthumb # Thumb-инструкции
        -mfpu=fpv4-sp-d16 # FPU single-precision
        -mfloat-abi=hard # аппаратный float ABI
        -ffunction-sections # секция на функцию — для --gc-sections
        -fdata-sections # секция на данные
    ) # конец флагов МК
endif() # конец ветки Generic
