cmake_minimum_required(VERSION 3.20) # нужна CMake 3.20 и новее
project(snmp_f411 C ASM) # проект на C, язык ASM оставлен на случай .s

set(CMAKE_C_STANDARD 11) # стандарт языка C11
set(CMAKE_C_STANDARD_REQUIRED ON) # не понижать стандарт молча

if(NOT CMAKE_BUILD_TYPE) # тип сборки не задан
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE) # оптимизация плюс отладка
endif() # конец выбора типа сборки

add_subdirectory(lib/snmp) # переносимая библиотека SNMP

if(CMAKE_SYSTEM_NAME STREQUAL "Generic") # кросс-компиляция под МК
    add_subdirectory(firmware) # прошивка STM32F411
else() # обычный хост
    enable_testing() # включить ctest
    add_subdirectory(tests) # хостовые тесты агента
endif() # конец выбора цели
