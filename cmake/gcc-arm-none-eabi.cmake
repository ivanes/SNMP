set(CMAKE_SYSTEM_NAME Generic) # голая система без ОС
set(CMAKE_SYSTEM_PROCESSOR arm) # архитектура ARM

set(CMAKE_C_COMPILER arm-none-eabi-gcc) # кросс-компилятор C
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc) # тот же gcc для ассемблера
set(CMAKE_OBJCOPY arm-none-eabi-objcopy) # утилита ELF→BIN/HEX
set(CMAKE_OBJDUMP arm-none-eabi-objdump) # дизассемблер
set(CMAKE_SIZE arm-none-eabi-size) # печать размеров секций

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) # не линковать пробные тесты CMake
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # программы искать на хосте
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) # библиотеки — только в sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY) # заголовки — только в sysroot
