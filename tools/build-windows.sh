#!/bin/bash
# Cross-compile the Windows build on Linux with llvm-mingw.
#
#   tools/build-windows.sh            (after tools/recompile.sh ROM)
#
# Needs: llvm-mingw (https://github.com/mstorsjo/llvm-mingw) in $LLVM_MINGW,
# DirectX-Headers (https://github.com/microsoft/DirectX-Headers) in
# $DIRECTX_HEADERS, wine64 (runs build-time tools), cmake and ninja.
# Optional: LAUNCHER_LOGO=path/to/logo.png to show the logo in the launcher.
# Optional: BACKGROUNDS_DIR=folder of TV background PNGs to bundle.
# Optional: HD_TEXTURE_PACK=path/to/pack.rtz to bundle the HD texture pack.
# Output: build/win/package/1080SnowboardingRecompiled (zip it to share).
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
: "${LLVM_MINGW:?set LLVM_MINGW to the llvm-mingw directory}"
: "${DIRECTX_HEADERS:?set DIRECTX_HEADERS to a DirectX-Headers checkout}"

# Small MinGW fixes to the renderer submodules (no-ops on a native Windows build).
apply_patch() {
    local dir=$1 patch=$2
    if git -C "$dir" apply --reverse --check "$patch" 2>/dev/null; then
        return  # already applied
    fi
    git -C "$dir" apply "$patch"
}
apply_patch "$ROOT/lib/rt64" "$ROOT/patches/windows-cross/rt64.patch"
apply_patch "$ROOT/lib/rt64/src/contrib/plume" "$ROOT/patches/windows-cross/plume.patch"

cmake -S "$ROOT" -B "$ROOT/build/win" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-clang.cmake" \
    -DLLVM_MINGW="$LLVM_MINGW" -DDIRECTX_HEADERS="$DIRECTX_HEADERS"
cmake --build "$ROOT/build/win"

PKG="$ROOT/build/win/package/1080SnowboardingRecompiled"
rm -rf "$PKG"
mkdir -p "$PKG"
cd "$ROOT/build/win"
cp 1080SnowboardingRecompiled.exe SDL2.dll dxcompiler.dll dxil.dll recompcontrollerdb.txt "$PKG/"
cp -r assets "$PKG/"
# The launcher shows the 1080 logo if it is packaged; it is not in the repository.
if [ -n "${LAUNCHER_LOGO:-}" ] && [ -f "$LAUNCHER_LOGO" ]; then
    cp "$LAUNCHER_LOGO" "$PKG/assets/launcher/logo.png"
fi
# The HD texture pack (Settings > Graphics > HD Textures) if one is given. It is
# made from the game's textures, so it is not in the repository either.
if [ -n "${HD_TEXTURE_PACK:-}" ] && [ -f "$HD_TEXTURE_PACK" ]; then
    mkdir -p "$PKG/textures"
    cp "$HD_TEXTURE_PACK" "$PKG/textures/1080-hd-textures.rtz"
fi
# TV backgrounds: an empty "backgrounds" folder players can drop PNGs into, plus
# any given in BACKGROUNDS_DIR (artwork is not in the repository).
mkdir -p "$PKG/backgrounds"
if [ -n "${BACKGROUNDS_DIR:-}" ] && [ -d "$BACKGROUNDS_DIR" ]; then
    find "$BACKGROUNDS_DIR" -maxdepth 1 -type f \( -iname '*.png' -o -iname '*.txt' \) -exec cp {} "$PKG/backgrounds/" \;
fi
VERSION=$(sed -n 's/^GAME_VERSION:STRING=//p' CMakeCache.txt)
sed "1s/(early)/(v$VERSION, early)/" "$ROOT/tools/windows-readme.txt" > "$PKG/README.txt"
"$LLVM_MINGW/bin/llvm-strip" "$PKG/1080SnowboardingRecompiled.exe"
echo "Packaged to $PKG"
