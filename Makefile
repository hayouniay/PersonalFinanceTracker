BUILD_DIR ?= build
DB ?= data/finance.db
CMAKE_FLAGS ?=

.PHONY: all configure build test run run-qt clean
all: build

configure:
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR) -j$$(nproc)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	./$(BUILD_DIR)/personal_finance_tracker $(DB)

run-qt: build
	./$(BUILD_DIR)/personal_finance_tracker_qt $(DB)

clean:
	rm -rf $(BUILD_DIR)
