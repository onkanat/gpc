# GPC - GPS Console Project Instructions

## Project Overview
GPC (GPS Console) is a professional GPS monitoring application built with C/C++, SDL2, OpenGL, and Dear ImGui. The project provides real-time GPS data visualization, interactive mapping, satellite tracking, and comprehensive GPS device management.

## Architecture
- **Main Application**: `src/gps_gui.c` - SDL2 + ImGui main interface
- **Modular Components**: Each GPS feature in separate modules under `src/include/`
- **Data Organization**: All GPS logs and exports stored in `data/` directory

## Key Modules
- `gps_data.h/c` - GPS data structures & NMEA parsing
- `gps_serial.h/c` - Serial communication & command interface
- `gps_map.h/c` - Interactive map system & track recording
- `gps_polar.h/c` - Satellite sky plot (polar coordinates)
- `gps_compass.h/c` - Digital compass & direction display
- `gps_console.h/c` - Raw NMEA console & command interface
- `minmea.h/c` - NMEA 0183 parsing library

## Build System
- **Primary**: `make` (uses Makefile in root)
- **Dependencies**: SDL2, OpenGL, cimgui (included)
- **Platforms**: macOS (primary), Linux (supported)

## UI Components
The application uses a dockable window system with 6 main panels:
1. **Telemetry** - GPS status, position, time data
2. **Map** - Interactive map with zoom, pan, track recording
3. **Satellites** - Satellite list with signal strength
4. **Sky Plot** - Polar satellite position view
5. **Compass** - Digital compass with GPS heading
6. **Raw Data Console** - NMEA command interface

## Data Management
- **GPS Logs**: `data/gps_log_YYYYMMDD_HHMMSS.nmea`
- **GPX Exports**: `data/gps_track.gpx`
- **Auto-creation**: `data/` directory created automatically

## Development Guidelines
- **Code Style**: C99/C++11 standards, modular design
- **Commits**: Feature-based commits with descriptive messages
- **Testing**: Test with real GPS hardware when possible
- **Documentation**: Keep README.md updated with new features

## Common Tasks
1. **Build**: `make clean && make`
2. **Run**: `./gps_gui`
3. **GPS Connection**: Menu → Connection → Connect...
4. **Start Logging**: Menu → Tools → Start Logging
5. **Export Track**: Menu → Tools → Export GPX...

## GPS Hardware Support
- **USB GPS Devices**: Auto-detection on macOS (/dev/cu.usbmodem*) and Linux (/dev/ttyUSB*, /dev/ttyACM*)
- **NMEA Protocol**: RMC, GGA, GSA, GSV, GLL, VTG, ZDA sentences
- **MTK Commands**: MediaTek GPS module configuration support
- **Baud Rates**: 4800-115200 (default 9600)

## Current Status
✅ Core GPS functionality complete
✅ Modular architecture implemented  
✅ Connection management system
✅ File organization (data/ directory)
✅ Real-time NMEA console
✅ Interactive mapping with track recording
✅ Comprehensive satellite monitoring

## Next Development Areas
- [ ] Enhanced UI controls and user experience
- [ ] Advanced track analysis tools
- [ ] Waypoint management system
- [ ] Offline map tiles support
- [ ] Multi-GPS device support