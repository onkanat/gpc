# Build & Compile Prompt

## Context
You are working on the GPC (GPS Console) project, a C/C++ application with SDL2, OpenGL, and Dear ImGui.

## Build Instructions

### Standard Build
```bash
make clean && make
```

### Debug Build
```bash
make clean && make CFLAGS="-g -O0"
```

### Verbose Build (if needed)
```bash
make clean && make VERBOSE=1
```

## Build System Details
- **Compiler**: g++ (treats .c files as C++ for ImGui compatibility)
- **Standards**: C99/C++11
- **Dependencies**: SDL2 (system), cimgui (included), OpenGL (framework)
- **Warnings**: -Wall -Wextra enabled
- **Output**: `gps_gui` executable in project root

## Common Build Issues & Solutions

### SDL2 Not Found (macOS)
```bash
brew install sdl2
```

### SDL2 Not Found (Linux)
```bash
sudo apt-get install libsdl2-dev libgl1-mesa-dev
```

### Clean Build After Changes
Always run `make clean` before `make` when:
- Adding new source files
- Changing header files
- Modifying build configuration
- After git pull/merge

### File Organization
- **Sources**: `src/gps_gui.c` + `src/include/*.c`
- **Headers**: `src/include/*.h`
- **ImGui**: `src/cimgui/` (bundled)
- **Output**: Root directory (`gps_gui`)

## Module Compilation
All modules compile together:
- gps_gui.c (main)
- minmea.c (NMEA parser)
- gps_data.c (GPS data structures)
- gps_serial.c (serial communication)
- gps_map.c (mapping system)
- gps_polar.c (satellite sky plot)
- gps_compass.c (digital compass)
- gps_console.c (raw data console)

## Quick Commands
```bash
# Full rebuild and run
make clean && make && ./gps_gui

# Build only if needed
make

# Check build dependencies
make --dry-run
```