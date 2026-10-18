.PHONY: all test firmware firmware-blackpill clean # цели не являются файлами

BUILD_HOST := build/host # каталог хостовой сборки
BUILD_FW := build/fw # каталог прошивки Nucleo
BUILD_FW_BP := build/fw-blackpill # каталог прошивки Black Pill

all: test firmware # по умолчанию тесты и Nucleo-прошивка

test: # собрать и прогнать хостовые тесты
	cmake -S . -B $(BUILD_HOST) # сгенерировать хостовый проект
	cmake --build $(BUILD_HOST) # скомпилировать test_snmp
	ctest --test-dir $(BUILD_HOST) --output-on-failure # выполнить проверки

firmware: # прошивка под NUCLEO-F411RE
	cmake -S . -B $(BUILD_FW) -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DBOARD=nucleo # кросс-CMake Nucleo
	cmake --build $(BUILD_FW) # собрать ELF/BIN/HEX

firmware-blackpill: # прошивка под WeAct Black Pill
	cmake -S . -B $(BUILD_FW_BP) -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DBOARD=blackpill # кросс-CMake Black Pill
	cmake --build $(BUILD_FW_BP) # собрать ELF/BIN/HEX

clean: # удалить каталоги сборки
	rm -rf build # стереть build/
