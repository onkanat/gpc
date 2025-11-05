# Debug & Troubleshooting Prompt

## Context
You are debugging the GPC (GPS Console) application, a real-time GPS monitoring tool.

## Debug Build & Run
```bash
# Compile with debug symbols
make clean && make CFLAGS="-g -O0"

# Run with verbose output
./gps_gui --verbose

# Run with debugger
gdb ./gps_gui
```

## Common Issues & Solutions

### GPS Connection Issues
1. **Port Access Problems**
   - Check USB connection
   - Verify port permissions: `ls -la /dev/tty*usb*` or `/dev/cu.*usb*`
   - Add user to dialout group (Linux): `sudo usermod -a -G dialout $USER`

2. **No GPS Devices Found**
   - Refresh ports in Connection Dialog
   - Check if device appears: `ls /dev/tty.usbmodem*` (macOS) or `/dev/ttyUSB*` (Linux)
   - Try different USB port/cable

3. **Resource Busy Error**
   - Another application may be using the GPS device
   - Close other GPS software
   - Restart GPS device

### Application Crashes
1. **ImGui/SDL2 Issues**
   - Check OpenGL driver support
   - Verify SDL2 installation: `sdl2-config --version`
   - Update graphics drivers

2. **Segmentation Faults**
   - Run with gdb: `gdb ./gps_gui`
   - Check for null pointer access
   - Verify GPS data structure initialization

### UI/Display Issues
1. **Blank Window**
   - OpenGL context creation failed
   - Check graphics card compatibility
   - Try windowed mode

2. **Missing Tabs/Panels**
   - Reset ImGui layout: delete `imgui.ini`
   - Check docking configuration
   - Verify window creation code

### Data Issues
1. **No NMEA Data in Console**
   - Verify GPS fix (satellites visible)
   - Check baud rate (try 9600, 4800)
   - Confirm NMEA sentence output from device

2. **Logging Not Working**
   - Check data/ directory permissions
   - Verify disk space
   - Ensure logging is enabled in Tools menu

## Debug Tools & Commands

### GPS Device Testing
```bash
# Test serial communication (macOS)
screen /dev/cu.usbmodem1201 9600

# Test serial communication (Linux)  
screen /dev/ttyUSB0 9600

# Monitor system messages
tail -f /var/log/system.log  # macOS
dmesg | tail                 # Linux
```

### Application State
- **Connection Status**: Check status bar
- **Log Files**: Check `data/` directory
- **Console Output**: Watch terminal for errors
- **ImGui State**: Delete `imgui.ini` to reset UI

### Memory/Performance
```bash
# Monitor resource usage
top -pid `pgrep gps_gui`

# Check for memory leaks (macOS)
leaks gps_gui

# Profile performance
time ./gps_gui
```

## Module-Specific Debug Points

### GPS Serial (gps_serial.c)
- Port opening success
- Baud rate configuration
- NMEA sentence parsing
- Buffer overflow checks

### GPS Data (gps_data.c)
- Structure initialization
- NMEA sentence validation
- Coordinate conversion
- Satellite data updates

### Map System (gps_map.c)
- Track point recording
- Coordinate transformations
- GPX export functionality

### Console (gps_console.c)
- Circular buffer management
- Command parsing
- Auto-scroll behavior

## Error Patterns
- **"Failed to open port"**: Permission or device issues
- **"Resource busy"**: Port already in use
- **"No GPS fix"**: Satellite acquisition needed
- **"OpenGL error"**: Graphics driver issues
- **Blank console**: NMEA data not flowing