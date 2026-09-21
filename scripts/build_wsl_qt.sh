#!/usr/bin/env bash

set -euo pipefail

sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    qtbase5-dev \
    qtbase5-dev-tools \
    libqt5charts5-dev \
    libsqlite3-dev

cmake -S . \
    -B build/build_qt \
    -G Ninja \
    -DBUILD_QT5_GUI=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build/build_qt --parallel

ctest \
    --test-dir build/build_qt \
    --output-on-failure

mkdir -p dist/dist_qt

cp build/build_qt/personal_finance_tracker_qt \
    dist/dist_qt/