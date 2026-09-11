#!/usr/bin/env bash
set -euo pipefail
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev qtbase5-dev-tools libsqlite3-dev
cmake -S . -B build -DBUILD_QT5_GUI=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
