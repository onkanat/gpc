# GPC - Agent Development Guidelines

## Build Commands
- **Build**: `make` (compiles to `gps_gui` binary)
- **Clean**: `make clean` (removes binary)
- **Debug build**: `make clean && make CFLAGS="-g -O0"`
- **Run**: `./gps_gui`

## Testing
- **cimgui test**: `cd src/cimgui/test && cmake . && make && ./cimgui_test`
- **GPS hardware testing**: Use Connection Dialog (Menu → Connection → Connect...)
- **No formal unit test framework** - test with real GPS hardware

## Code Style Guidelines
- **Language**: C99 for GPS modules, C++11 for ImGui/cimgui components
- **Naming**: `snake_case` for functions/variables, `PascalCase` for types
- **Headers**: Include guards with `#ifndef MODULE_H #define MODULE_H #endif`
- **Comments**: Turkish comments in main files, English in new code preferred
- **Structure**: Modular design - each GPS feature in separate .h/.c pair under `src/include/`

## Module Organization
- `gps_data.h/c` - Core GPS data structures & NMEA parsing
- `gps_serial.h/c` - Serial communication & command interface  
- `gps_map.h/c` - Interactive map system & track recording
- `gps_polar.h/c` - Satellite sky plot (polar coordinates)
- `gps_compass.h/c` - Digital compass & direction display
- `gps_console.h/c` - Raw NMEA console & command interface
- `tools.h/c` - Utility functions

## Error Handling
- Use return codes for function errors
- Check GPS data validity with `bool valid` fields
- Log errors to console, maintain connection status
- Handle serial port failures gracefully

## Dependencies
- **SDL2**: GUI framework (`sdl2-config --cflags --libs`)
- **OpenGL**: Graphics rendering (`-framework OpenGL` on macOS)
- **cimgui**: Dear ImGui C bindings (included in `src/cimgui/`)
- **minmea**: NMEA parsing library (included in `src/include/`)

## Data Management
- **GPS logs**: `data/gps_log_YYYYMMDD_HHMMSS.nmea`
- **GPX exports**: `data/gps_track.gpx`
- **Auto-creation**: `data/` directory created automatically