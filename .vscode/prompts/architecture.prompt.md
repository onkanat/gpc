# Code Architecture & Development Prompt

## Context
You are working on the GPC (GPS Console) codebase, which follows a modular architecture pattern.

## Project Structure
```
gpc/
├── src/
│   ├── gps_gui.c           # Main application & UI
│   └── include/            # Modular components
│       ├── gps_data.h/c    # GPS data structures
│       ├── gps_serial.h/c  # Serial communication
│       ├── gps_map.h/c     # Map & tracking
│       ├── gps_polar.h/c   # Satellite sky plot  
│       ├── gps_compass.h/c # Digital compass
│       ├── gps_console.h/c # Raw NMEA console
│       └── minmea.h/c      # NMEA parsing
├── data/                   # GPS logs & exports
└── .vscode/               # Development tools
```

## Coding Standards

### C/C++ Guidelines
- **Standards**: C99 for logic, C++11 for ImGui compatibility
- **Naming**: snake_case for functions/variables, PascalCase for types
- **Headers**: Include guards for all .h files
- **Memory**: Explicit init/cleanup functions for all modules
- **Error Handling**: Return false/NULL on errors, true on success

### Function Patterns
```c
// Module initialization
bool module_init(module_t* module);
void module_cleanup(module_t* module);

// Data operations  
bool module_update(module_t* module, const input_data_t* input);
const char* module_get_status(const module_t* module);

// UI rendering
void render_module_window(app_state_t* app);
void render_module_panel(app_state_t* app);
```

### File Organization
- **Main UI**: `gps_gui.c` - SDL2/ImGui setup, main loop, UI rendering
- **Modules**: Each feature as separate .h/.c pair in `include/`
- **Headers**: Interface definitions, no implementation
- **Implementation**: All logic in .c files

## Modular Architecture

### GPS Data Module (gps_data.h/c)
- **Purpose**: GPS data structures, NMEA parsing integration
- **Key Types**: `gps_data_t`, `satellite_info_t`
- **Functions**: `gps_data_init()`, `gps_data_update_from_*()`, status helpers

### GPS Serial Module (gps_serial.h/c)  
- **Purpose**: Serial port communication, NMEA command sending
- **Key Types**: `gps_serial_t`
- **Functions**: `gps_serial_open()`, `gps_serial_read_data()`, `gps_serial_send_command()`

### GPS Map Module (gps_map.h/c)
- **Purpose**: Interactive mapping, track recording, GPX export
- **Key Types**: `map_system_t`, `track_point_t`
- **Functions**: `map_system_update()`, `map_system_save_gpx()`

### GPS Polar Module (gps_polar.h/c)
- **Purpose**: Satellite sky plot in polar coordinates
- **Key Types**: `polar_view_t`
- **Functions**: `polar_view_update()`, satellite positioning

### GPS Compass Module (gps_compass.h/c)
- **Purpose**: Digital compass, heading calculations
- **Key Types**: `compass_t`
- **Functions**: `compass_update()`, `compass_get_true_heading()`

### GPS Console Module (gps_console.h/c)
- **Purpose**: Raw NMEA console, command interface
- **Key Types**: `console_t`
- **Functions**: `console_add_line()`, `console_get_line()`

## UI Architecture

### Window System
- **Main Window**: Full-screen dockspace
- **Dockable Panels**: Each module as separate window
- **Status Bar**: Fixed bottom bar with connection info

### Rendering Pattern
```c
void render_module_window(app_state_t* app) {
    igSetNextWindowSize(...);  // Set default size
    if (igBegin("Module Name", NULL, flags)) {
        render_module_panel(app);  // Call panel renderer
    }
    igEnd();
}

void render_module_panel(app_state_t* app) {
    // Actual UI content here
    // No igBegin/igEnd - already in window context
}
```

## Development Patterns

### Adding New Features
1. **Create Module**: Add .h/.c files in `include/`
2. **Define Interface**: Public functions in header
3. **Implement Logic**: Core functionality in .c file
4. **Integrate UI**: Add window/panel renderers
5. **Update Makefile**: Include new source files
6. **Test Integration**: Verify build and functionality

### State Management
- **App State**: Central `app_state_t` structure in `gps_gui.c`
- **Module State**: Each module has own state structure
- **UI State**: Separate from business logic
- **Initialization**: All modules initialized in main()

### Memory Management
- **Stack Allocation**: Prefer stack for small/temporary data
- **Heap Allocation**: Only when necessary, with explicit cleanup
- **Strings**: Fixed-size buffers, `strncpy()` for safety
- **Cleanup**: Explicit cleanup functions for all modules

## Code Quality

### Error Handling
- **Return Values**: bool for success/failure, NULL for errors
- **Logging**: printf() for important events
- **Validation**: Check all input parameters
- **Graceful Degradation**: Continue operation when possible

### Performance
- **Real-time**: 60fps UI updates
- **GPS Updates**: Process NMEA data efficiently  
- **Memory**: Minimal allocations in main loop
- **UI**: Only redraw when data changes

### Maintainability
- **Separation of Concerns**: Each module has single responsibility
- **Loose Coupling**: Modules communicate through well-defined interfaces
- **Documentation**: Header comments for public functions
- **Testing**: Manual testing with real GPS hardware