BUILD_DIR ?= build
DIST_DIR ?= dist

CLI_BUILD_DIR := $(BUILD_DIR)/build_cli
QT_BUILD_DIR := $(BUILD_DIR)/build_qt

CLI_DIST_DIR := $(DIST_DIR)/dist_cli
QT_DIST_DIR := $(DIST_DIR)/dist_qt

CONSOLE_BIN := $(CLI_DIST_DIR)/personal_finance_tracker
QT_BIN := $(QT_DIST_DIR)/personal_finance_tracker_qt

DB ?= data/finance.db

CMAKE_FLAGS ?=

.PHONY: all configure build build-cli build-qt test run run-qt clean

all: build

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

configure: clean
	cmake -S . -B $(CLI_BUILD_DIR) $(CMAKE_FLAGS) -DBUILD_QT5_GUI=OFF
	cmake -S . -B $(QT_BUILD_DIR) $(CMAKE_FLAGS) -DBUILD_QT5_GUI=ON

build: configure
	cmake --build $(CLI_BUILD_DIR) -j$$(nproc)
	cmake --build $(QT_BUILD_DIR) -j$$(nproc)

	mkdir -p $(CLI_DIST_DIR) $(QT_DIST_DIR)

	cp $(CLI_BUILD_DIR)/personal_finance_tracker \
		$(CONSOLE_BIN)

	@if [ -x "$(QT_BUILD_DIR)/personal_finance_tracker_qt" ]; then \
		cp $(QT_BUILD_DIR)/personal_finance_tracker_qt \
			$(QT_BIN); \
	else \
		echo "Qt5 GUI binary not found; install Qt5 Widgets + Charts development packages."; \
		exit 1; \
	fi

build-cli:
	cmake -S . -B $(CLI_BUILD_DIR) \
		$(CMAKE_FLAGS) \
		-DBUILD_QT5_GUI=OFF \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(CLI_BUILD_DIR) -j$$(nproc)

	mkdir -p $(CLI_DIST_DIR)
	cp $(CLI_BUILD_DIR)/personal_finance_tracker \
		$(CONSOLE_BIN)

build-qt:
	cmake -S . -B $(QT_BUILD_DIR) \
		$(CMAKE_FLAGS) \
		-DBUILD_QT5_GUI=ON \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(QT_BUILD_DIR) -j$$(nproc)

	mkdir -p $(QT_DIST_DIR)

	@if [ -x "$(QT_BUILD_DIR)/personal_finance_tracker_qt" ]; then \
		cp $(QT_BUILD_DIR)/personal_finance_tracker_qt \
			$(QT_BIN); \
	else \
		echo "Qt5 GUI binary not found; install Qt5 Widgets + Charts development packages."; \
		exit 1; \
	fi

test: build
	ctest --test-dir $(CLI_BUILD_DIR) --output-on-failure
	ctest --test-dir $(QT_BUILD_DIR) --output-on-failure

run: build
	@if [ ! -x "$(CONSOLE_BIN)" ]; then \
		echo "Console application not found in $(CLI_DIST_DIR). Run 'make build' first."; \
		exit 1; \
	fi
	./$(CONSOLE_BIN) $(DB)

run-qt: build
	@if [ ! -x "$(QT_BIN)" ]; then \
		echo "Qt5 application not found in $(QT_DIST_DIR). Run 'make build' first."; \
		exit 1; \
	fi
	./$(QT_BIN) $(DB)
