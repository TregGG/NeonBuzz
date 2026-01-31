#!/bin/bash
# Script to download ImGui headers

IMGUI_VERSION="1.90.1"
THIRD_PARTY_DIR="third_party"

echo "Downloading ImGui..."
mkdir -p "$THIRD_PARTY_DIR"
cd "$THIRD_PARTY_DIR"

# Download ImGui
if [ ! -d "imgui" ]; then
    git clone --depth 1 --branch v$IMGUI_VERSION https://github.com/ocornut/imgui.git
else
    echo "ImGui already downloaded"
fi

cd ..
echo "Setup complete!"
