#!/usr/bin/env bash

set -euo pipefail

sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    libsqlite3-dev

cmake -S . \
    -B build/build_cli \
    -G Ninja \
    -DBUILD_QT5_GUI=OFF \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build/build_cli --parallel

ctest \
    --test-dir build/build_cli \
    --output-on-failure

mkdir -p dist/dist_cli

cp build/build_cli/personal_finance_tracker \
    dist/dist_cli/

echo
echo "Build and tests completed successfully."
echo "Console binary: dist/dist_cli/personal_finance_tracker"
