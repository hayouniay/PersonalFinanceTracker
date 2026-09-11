#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt install -y build-essential cmake libsqlite3-dev
make
make test

echo
echo "Build and tests completed successfully."
echo "Run the application with: make run"
