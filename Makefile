BUILD_DIR ?= build
DIST_DIR := dist
CONSOLE_BIN := $(DIST_DIR)/personal_finance_tracker
QT_BIN := $(DIST_DIR)/personal_finance_tracker_qt
DB ?= data/finance.db
CMAKE_FLAGS ?=

.PHONY: all configure build test run run-qt clean
all: build

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)

configure: clean
	cmake -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	cmake --build $(BUILD_DIR) -j$$(nproc)
	mkdir -p $(DIST_DIR)
	cp $(BUILD_DIR)/personal_finance_tracker $(CONSOLE_BIN)
	cp $(BUILD_DIR)/personal_finance_tracker_qt $(QT_BIN)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	@if [ ! -x "$(CONSOLE_BIN)" ]; then \
		echo "Application not found in $(DIST_DIR). Run 'make build' first."; \
		exit 1; \
	fi
	./$(CONSOLE_BIN)

run-qt: build
	@if [ ! -x "$(QT_BIN)" ]; then \
		echo "Qt5 Application not found in $(DIST_DIR). Run 'make build' first."; \
		exit 1; \
	fi
	./$(QT_BIN) $(DB)
