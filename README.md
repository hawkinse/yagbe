# Yagbe #

Yagbe (Yet Another Gameboy Emulator) is an emulator for the original 1989 Nintendo Gameboy and 1998 Gameboy Color written in C++, with SDL 2 for input, rendering and sound. It currently can run many games without link cable support. Game roms can be ran by themselves, or with a system boot rom for slightly higher compatibility and GBC colorization of black and white games.

### Arguments ###
* **-r /path/to/rom.gb** Path to rom image.
* **-b /path/to/bootrom** Optional. Path to a DMG/MGB/GBC boot rom.
* **-s n** Optional. Scales the window size by n from default 160x144.
* **-dmg** Run games in original Gameboy mode, regardless of GBC support
* **-gbc** Run games in Gameboy Color mode, regardless of GBC support.

### Controls ###
* **D-Pad** - Arrow keys
* **A** - X
* **B** - Z
* **Start** - Enter
* **Select** - Right Shift
* **1-4** - Toggle Audio Channels
* **F1** - Lower clock multiplier
* **F2** - Raise clock multiplier

### Linux Build Instructions ###
* cmake -G "Unix Makefiles"
* make

### Mac OS Build Instructions ###
* Install SDL2 and CMake with Brew
* cmake -G "Xcode"
* Open yagbe.xcodeproj
* Command + B

### Windows Build Instructions ###
* Launch CMake GUI
* Enter path to source, desired project location.
* Add a file path entry for **SDL2MAIN_LIBRARY** with the path to **SDL2main.lib**
* Add a file path entry for **SDL2_LIBRARY_TEMP** with the path to **SDL2.lib**
* Add a path for **SDL2_INCLUDE_DIR** with the path to the SDL2 **include** directory.
* Click configure.
* Select "Visual Studio 15 2017 Win64" from the list and click Finish.
* Click generate.
* Open the newly created .sln file.
* Build > Build Solution.
* When running, make sure SDL2.dll is in the same directory as yagbe.exe.
