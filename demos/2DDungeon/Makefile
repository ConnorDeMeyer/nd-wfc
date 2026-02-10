BUILD_DIR := build
BUILD_TYPE ?= Debug

.PHONY: all clean run configure build

all: build

configure:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/bin/dungeon_demo

clean:
	rm -rf $(BUILD_DIR)
