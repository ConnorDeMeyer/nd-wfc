BUILD_DIR := build
BUILD_TYPE ?= Release

.PHONY: all clean run test benchmark configure build

all: build

configure:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j$(shell nproc)

run: build
	./$(BUILD_DIR)/bin/sudoku_wfc_demo

test: build
	@if [ -f ./$(BUILD_DIR)/bin/sudoku_tests ]; then \
		./$(BUILD_DIR)/bin/sudoku_tests; \
	else \
		echo "Tests not available (Google Test not found)"; \
	fi

benchmark: build
	@if [ -f ./$(BUILD_DIR)/bin/sudoku_benchmarks ]; then \
		./$(BUILD_DIR)/bin/sudoku_benchmarks; \
	else \
		echo "Benchmarks not available (Google Benchmark not found)"; \
	fi

clean:
	rm -rf $(BUILD_DIR)
