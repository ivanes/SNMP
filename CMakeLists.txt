cmake_minimum_required(VERSION 3.20)
project(snmp_f411 C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

add_subdirectory(lib/snmp)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    add_subdirectory(firmware)
else()
    enable_testing()
    add_subdirectory(tests)
endif()
