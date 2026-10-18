add_library(snmp STATIC
    src/snmp.c
    src/frame.c
)

target_include_directories(snmp PUBLIC include)
target_compile_options(snmp PRIVATE -Wall -Wextra -Wpedantic)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    target_compile_options(snmp PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -ffunction-sections
        -fdata-sections
    )
endif()
