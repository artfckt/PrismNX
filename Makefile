ifeq ($(strip $(DEVKITPRO)),)
$(error Set DEVKITPRO to the devkitPro installation)
endif

export PATH := $(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PATH)
SHELL := /bin/bash
CC := aarch64-none-elf-gcc
CXX := aarch64-none-elf-g++
FIZEAU := third_party/fizeau/common
TESLA := build/tesla/include
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
INCLUDES := -Iinclude -isystem $(FIZEAU)/include -isystem $(TESLA) -isystem $(DEVKITPRO)/libnx/include
FLAGS := $(ARCH) -O2 -g -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections -D__SWITCH__ $(INCLUDES)
CXXFLAGS := $(FLAGS) -std=gnu++20 -fno-exceptions
CFLAGS := $(FLAGS) -std=gnu11
SOURCES := $(wildcard source/*.cpp)
OBJECTS := $(patsubst source/%.cpp,build/%.o,$(SOURCES)) build/fizeau.o

.PHONY: all test
all: out/SwitchColor.ovl

build/%.o: source/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/tesla/include/tesla.hpp: third_party/fizeau/lib/libtesla/include/tesla.hpp scripts/prepare_tesla.py
	python3 scripts/prepare_tesla.py

build/main.o build/tesla_impl.o build/ui.o build/color_ui.o build/system_ui.o build/tools_ui.o: build/tesla/include/tesla.hpp

build/fizeau.o: $(FIZEAU)/src/fizeau.c
	@mkdir -p build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

out/SwitchColor.elf: $(OBJECTS)
	@mkdir -p out
	$(CXX) $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -g -Wl,-Map,build/SwitchColor.map $(OBJECTS) -L$(DEVKITPRO)/libnx/lib -lnx -o $@

out/SwitchColor.nacp: Makefile
	@mkdir -p out
	nacptool --create "SwitchColor" "SwitchColor contributors" "0.2.0" $@

out/SwitchColor.ovl: out/SwitchColor.elf out/SwitchColor.nacp
	elf2nro $< $@ --nacp=out/SwitchColor.nacp

test:
	@mkdir -p build
	g++ -std=gnu++20 -O1 -g -Wall -Wextra -Werror -DSC_STORAGE_LINK_WRAPS -Itests/stubs -Iinclude -I$(FIZEAU)/include source/model.cpp source/presets.cpp source/backend.cpp source/storage.cpp tests/main.cpp tests/storage_test.cpp -Wl,--wrap=rename,--wrap=fwrite -o build/tests.exe
	./build/tests.exe

-include $(OBJECTS:.o=.d)
