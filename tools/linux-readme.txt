1080 Snowboarding: Recompiled 1.0.1 for Linux (experimental)

This Linux build is new and has not been tested on real machines yet, so it
may not work everywhere. The Windows build is the main one.

1. Extract this folder anywhere.
2. Run ./1080SnowboardingRecompiled.sh (or double-click it, if your file
   manager runs scripts).
3. Choose "Load ROM" and pick your own 1080 Snowboarding (USA) .z64 ROM.
   It is checked against the expected hash and copied into the app's data folder.
4. Choose "Start Game".

Needs a 64-bit distribution about as new as Ubuntu 24.04 or Fedora 40
(glibc 2.39), a GPU with Vulkan drivers, and these libraries, which most
desktops already have: SDL2, GTK 3 and FreeType. On Ubuntu or Debian:
    sudo apt install libsdl2-2.0-0 libgtk-3-0 libfreetype6 libvulkan1

Settings, controls, saves and log.txt live in ~/.config/1080SnowboardingRecompiled.
To keep everything next to the program instead, create an empty file named
portable.txt here.

The ROM is not included.
