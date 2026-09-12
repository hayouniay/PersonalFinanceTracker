# Personal Finance Tracker

C++17 personal finance application with **both a Linux/WSL console interface and a Qt5 desktop interface**. Both applications share the same finance engine and SQLite database.

## Features

- Income and expense transactions
- Categories and budgets
- Multiple accounts and transfers
- Transaction search and CSV import/export
- Monthly dashboard and SVG charts
- Savings goals
- Recurring transactions
- Forecasting
- Financial analytics
- Qt5 graphical interface
- Original console interface retained

## WSL2 dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev qtbase5-dev-tools libsqlite3-dev
```

Current Windows 11 WSLg is recommended for running the Qt GUI.

## Build both applications with CMake

```bash
cmake -S . -B build -DBUILD_QT5_GUI=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Executables:

```bash
./build/personal_finance_tracker       # console
./build/personal_finance_tracker_qt    # Qt5 GUI
```

You can optionally provide another SQLite database path:

```bash
./build/personal_finance_tracker_qt data/finance.db
```

## Makefile

```bash
make
make test
make run       # console version
make run-qt    # Qt5 version
```

## qmake

A Qt5 `.pro` project is also included:

```bash
qmake PersonalFinanceTrackerQt.pro
make -j$(nproc)
./personal_finance_tracker_qt
```

## Console version

The original console application is **not replaced**. It remains available as `personal_finance_tracker` and continues to expose the complete Phase 1–4 functionality.

## Shared database

The console and Qt5 applications use the same `data/finance.db`. Changes made by either interface are therefore persisted in the same SQLite database.

## Project structure

```text
PersonalFinanceTracker/
├── CMakeLists.txt
├── Makefile
├── PersonalFinanceTrackerQt.pro
├── build_wsl.sh
├── build_wsl_qt.sh
├── include/
│   ├── core/
│   ├── services/
│   ├── storage/
│   ├── ui/
│   ├── utils/
│   └── qt/
├── src/
│   ├── core/
│   ├── services/
│   ├── storage/
│   ├── ui/              # original console UI
│   ├── utils/
│   └── qt/              # Qt5 GUI
├── tests/
├── data/
├── reports/
└── docs/
```

## Version

1.1.0 — Phase 4 engine + Qt5 desktop interface.
