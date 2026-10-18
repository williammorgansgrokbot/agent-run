#!/usr/bin/env bash
# Restores the third-party folders not committed to the repo (components/, external/).
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
git clone --recursive --depth 1 --shallow-submodules https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template "$T/tpl"
mkdir -p components external
cp -rn "$T/tpl/components/." components/
cp -rn "$T/tpl/external/." external/
[ -d components/TFT_eSPI ] || git clone --depth 1 https://github.com/Bodmer/TFT_eSPI components/TFT_eSPI
rm -f components/TFT_eSPI/Kconfig            # board config comes from main/CYD4_Setup.h instead
cp tft_espi_CMakeLists.txt components/TFT_eSPI/CMakeLists.txt
rm -rf "$T"
echo "Dependencies ready. Build with: pio run -e esp32dev"
