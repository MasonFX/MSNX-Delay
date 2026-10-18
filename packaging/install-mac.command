#!/bin/bash
# Installs MSNX Delay into your user plugin folders. Double-click to run.

cd "$(dirname "$0")" || exit 1

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"

echo "Installing MSNX Delay..."
mkdir -p "$VST3_DIR" "$AU_DIR"

rm -rf "$VST3_DIR/MSNX Delay.vst3" "$AU_DIR/MSNX Delay.component"

# Remove the old version (it was called "MPC3K Delay") so the DAW doesn't see two copies.
rm -rf "$VST3_DIR/MPC3K Delay.vst3" "$AU_DIR/MPC3K Delay.component"
cp -R "MSNX Delay.vst3"      "$VST3_DIR/"
cp -R "MSNX Delay.component" "$AU_DIR/"

# Tell macOS these downloaded files are OK to load.
xattr -dr com.apple.quarantine "$VST3_DIR/MSNX Delay.vst3" "$AU_DIR/MSNX Delay.component" 2>/dev/null
xattr -dr com.apple.quarantine "MSNX Delay.app" 2>/dev/null

# Make macOS notice the new Audio Unit without a reboot.
killall -9 AudioComponentRegistrar 2>/dev/null

echo ""
echo "Done! Restart your DAW and look for \"MSNX Delay\"."
echo "You can close this window."
