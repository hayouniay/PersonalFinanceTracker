# Personal Finance Tracker — Qt5 + Console

This project keeps the original console application and adds a Qt5 desktop GUI. Both applications use the same C++17 finance engine and SQLite database.

## WSL2 / Ubuntu dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev qtbase5-dev-tools libsqlite3-dev
```

For WSL2 GUI display, use WSLg (included with current Windows 11 WSL). On older setups, configure an X server separately.

## CMake build

```bash
cmake -S . -B build -DBUILD_QT5_GUI=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Executables:

```bash
./build/personal_finance_tracker
./build/personal_finance_tracker_qt
```

## Makefile

```bash
make
make test
make run       # console
make run-qt    # Qt5 GUI
```

## qmake

```bash
qmake PersonalFinanceTrackerQt.pro
make -j$(nproc)
./personal_finance_tracker_qt
```

The GUI and console share `data/finance.db`, so changes made in one are visible to the other after refresh/restart.

## Professional Qt5 interface

Version 4.2 introduces a more production-oriented desktop UI:

- persistent left navigation instead of a basic tab strip
- dashboard cards for income, expenses, net cash flow and savings rate
- account and savings-goal snapshots on the dashboard
- consistent panels, tables, spacing, typography and status feedback
- clearer primary/secondary/danger actions
- improved transaction, budget, goal and recurring-transaction forms
- budget status indicators and savings-goal progress values
- application-wide Qt stylesheet with a restrained business dashboard look
- console application remains unchanged and available as `personal_finance_tracker`

The GUI continues to use the same SQLite database and business logic as the console application.
