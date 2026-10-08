#!/usr/bin/env bash
set -euo pipefail
# Run from the project root. Build the VST3; Linux legacy adapter is not installed.
if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake git pkg-config \
        libasound2-dev libfreetype6-dev libfontconfig1-dev libx11-dev libxinerama-dev \
        libxrandr-dev libxcursor-dev libxcomposite-dev libgl1-mesa-dev
fi
cmake -S . -B build_linux -DCMAKE_BUILD_TYPE=Release -DMETTAVOXAD_BUILD_LEGACY=OFF
cmake --build build_linux --config Release --parallel 2
bundle="build_linux/mettavoxad_artefacts/Release/VST3/METTAVOXAD21.vst3"
destination="$HOME/.vst3/METTAVOXAD21.vst3"
mkdir -p "$destination"
cp -a "$bundle/." "$destination/"
echo "Concluido. Reescaneie ~/.vst3 no host."
