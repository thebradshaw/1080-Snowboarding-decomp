#!/bin/bash
# Packages the Linux build (build/pc) into build/linux-package/, ready to tar.
# Optional: HD_TEXTURE_PACK=path/to/pack.rtz, LAUNCHER_LOGO=path/to/logo.png and
# BACKGROUNDS_DIR=folder of TV background PNGs
# (none of these are in the repository).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="$ROOT/build/linux-package/1080SnowboardingRecompiled"
rm -rf "$PKG"
mkdir -p "$PKG"
cp "$ROOT/build/pc/1080SnowboardingRecompiled" "$PKG/"
strip "$PKG/1080SnowboardingRecompiled"
cp -r "$ROOT/build/pc/assets" "$PKG/"
rm -f "$PKG/assets/launcher/logo.png"
cp "$ROOT/recompcontrollerdb.txt" "$PKG/"
cp "$ROOT/tools/linux-start.sh" "$PKG/1080SnowboardingRecompiled.sh"
chmod +x "$PKG/1080SnowboardingRecompiled.sh"
cp "$ROOT/tools/linux-readme.txt" "$PKG/README.txt"
if [ -n "${HD_TEXTURE_PACK:-}" ] && [ -f "$HD_TEXTURE_PACK" ]; then
    mkdir -p "$PKG/textures"
    cp "$HD_TEXTURE_PACK" "$PKG/textures/1080-hd-textures.rtz"
fi
if [ -n "${LAUNCHER_LOGO:-}" ] && [ -f "$LAUNCHER_LOGO" ]; then
    cp "$LAUNCHER_LOGO" "$PKG/assets/launcher/logo.png"
fi
# TV backgrounds: an empty "backgrounds" folder players can drop PNGs into, plus
# any given in BACKGROUNDS_DIR (artwork is not in the repository).
mkdir -p "$PKG/backgrounds"
if [ -n "${BACKGROUNDS_DIR:-}" ] && [ -d "$BACKGROUNDS_DIR" ]; then
    find "$BACKGROUNDS_DIR" -maxdepth 1 -type f \( -iname '*.png' -o -iname '*.txt' \) -exec cp {} "$PKG/backgrounds/" \;
fi
echo "Packaged to $PKG"
