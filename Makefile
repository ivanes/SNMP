.PHONY: all test firmware firmware-blackpill clean

BUILD_HOST := build/host
BUILD_FW := build/fw
BUILD_FW_BP := build/fw-blackpill

all: test firmware

test:
	cmake -S . -B $(BUILD_HOST)
	cmake --build $(BUILD_HOST)
	ctest --test-dir $(BUILD_HOST) --output-on-failure

firmware:
	cmake -S . -B $(BUILD_FW) \
		-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
		-DBOARD=nucleo
	cmake --build $(BUILD_FW)

firmware-blackpill:
	cmake -S . -B $(BUILD_FW_BP) \
		-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
		-DBOARD=blackpill
	cmake --build $(BUILD_FW_BP)

clean:
	rm -rf build
