# UI/UX Development Prompt

## Context
You are improving the user interface and user experience of the GPC (GPS Console) application using Dear ImGui.

## UI Architecture Overview

### Window System
- **Main Dockspace**: Full-screen docking area
- **Dockable Panels**: 6 main windows (Telemetry, Map, Satellites, Sky Plot, Compass, Raw Data)
- **Modal Dialogs**: Connection dialog, GPX export dialog
- **Menu Bar**: Connection, Tools, Help menus
- **Status Bar**: Fixed bottom bar with connection info

### Current UI Components

### Telemetry Panel
- **GPS Status**: Connection state, fix quality, satellite count
- **Position Data**: Latitude, longitude, altitude
- **Motion Data**: Speed, course, time
- **Quality Metrics**: DOP values, accuracy indicators

### Map Panel
- **Interactive Map**: Zoom, pan, track display
- **Controls**: Zoom buttons, auto-center, fit-to-track
- **Track Recording**: Start/stop recording, visual track
- **Navigation**: Real-time position indicator

### Satellite Panels
- **List View**: PRN, elevation, azimuth, SNR table
- **Sky Plot**: Polar coordinate satellite positions
- **Signal Quality**: Color-coded signal strength
- **Selection**: Click satellite for details

### Compass Panel
- **Digital Compass**: Circular compass display
- **Direction Arrow**: Red arrow showing heading
- **Cardinal Directions**: N, NE, E, SE, S, SW, W, NW labels
- **Controls**: Auto-rotate toggle, declination slider

### Raw Data Console
- **NMEA Display**: Last 5 sentences with color coding
- **Command Input**: Send commands to GPS device
- **Auto-scroll**: Optional automatic scrolling
- **Predefined Commands**: Common GPS commands buttons

## UI/UX Improvement Areas

### Visual Design
1. **Color Scheme**
   - Consistent color palette across panels
   - High contrast for readability
   - Color coding for data status (good/warning/error)
   - Dark/light theme support

2. **Typography**
   - Consistent font sizing hierarchy
   - Monospace for data display
   - Bold/italic for emphasis
   - Readable fonts for GPS coordinates

3. **Layout & Spacing**
   - Consistent padding and margins
   - Logical grouping of related elements
   - Responsive layout for different window sizes
   - Clear visual hierarchy

### User Experience
1. **Workflow Optimization**
   - Streamlined GPS connection process
   - Quick access to common functions
   - Keyboard shortcuts for power users
   - Context-sensitive help

2. **Error Handling**
   - Clear error messages
   - Helpful suggestions for problem resolution
   - Non-blocking error notifications
   - Recovery suggestions

3. **Data Visualization**
   - Clear, easy-to-read GPS coordinates
   - Visual indicators for signal quality
   - Intuitive map controls
   - Real-time data updates

## ImGui Best Practices

### Layout Patterns
```c
// Group related controls
if (igCollapsingHeader("GPS Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
    igIndent();
    // Settings controls here
    igUnindent();
}

// Use columns for data display
if (igBeginTable("GPSData", 2, ImGuiTableFlags_Borders)) {
    igTableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);
    igTableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
    // Table rows here
    igEndTable();
}
```

### Color and Style
```c
// Status-based colors
ImVec4 get_status_color(gps_status_t status) {
    switch (status) {
        case GPS_STATUS_CONNECTED: return (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
        case GPS_STATUS_ERROR: return (ImVec4){1.0f, 0.0f, 0.0f, 1.0f}; // Red
        case GPS_STATUS_DISCONNECTED: return (ImVec4){0.7f, 0.7f, 0.7f, 1.0f}; // Gray
    }
}

// Apply styles consistently
igPushStyleColor_Vec4(ImGuiCol_Text, get_status_color(gps->status));
igText("Status: %s", status_text);
igPopStyleColor(1);
```

### Interactive Elements
```c
// Helpful tooltips
if (igIsItemHovered(0)) {
    igSetTooltip("Click to refresh available GPS ports");
}

// Disabled state for unavailable actions
if (!can_perform_action) igBeginDisabled(true);
if (igButton("Action", (ImVec2){0, 0})) {
    // Action code
}
if (!can_perform_action) igEndDisabled();
```

## Specific Improvement Ideas

### Connection Management
- **Port Auto-refresh**: Automatically detect new GPS devices
- **Connection History**: Remember previously used ports/settings
- **Connection Status Icons**: Visual indicators in status bar
- **Quick Connect**: One-click connection to last used device

### Map Interface
- **Zoom Level Display**: Show current zoom level
- **Scale Bar**: Distance scale on map
- **Coordinate Display**: Mouse-over coordinates
- **Waypoint Markers**: Add/edit waypoints on map

### Data Display
- **Units Toggle**: Switch between metric/imperial
- **Precision Control**: Adjustable coordinate precision
- **Time Zones**: Local time vs GPS time display
- **Data Export**: Quick copy coordinates to clipboard

### Satellite Monitoring
- **SNR Graphs**: Historical signal strength graphs
- **Satellite Health**: Health status indicators
- **Constellation Filter**: Filter by GPS/GLONASS/Galileo
- **Acquisition Progress**: Visual progress for satellite acquisition

### Console Enhancements
- **Command History**: Up/down arrow for command history
- **Syntax Highlighting**: Color-code NMEA commands
- **Command Templates**: Predefined command shortcuts
- **Response Parsing**: Parse and highlight command responses

## Accessibility Considerations

### Visual Accessibility
- **High Contrast Mode**: Support for high contrast displays
- **Font Scaling**: Adjustable font sizes
- **Color Blind Support**: Patterns/shapes in addition to colors
- **Screen Reader**: Proper labeling for screen readers

### Motor Accessibility
- **Keyboard Navigation**: Full keyboard access to all functions
- **Large Click Targets**: Appropriately sized buttons
- **Drag and Drop**: Alternative to precise mouse operations
- **Voice Control**: Consider voice command integration

### Cognitive Accessibility
- **Consistent Layout**: Predictable interface layout
- **Clear Labels**: Descriptive button and field labels
- **Progress Indicators**: Show progress for long operations
- **Help System**: Context-sensitive help and tutorials

## Performance Considerations

### Rendering Optimization
- **Conditional Rendering**: Only render visible panels
- **Data Throttling**: Limit update frequency for smooth UI
- **Lazy Loading**: Load heavy resources when needed
- **Memory Management**: Efficient ImGui resource usage

### Responsiveness
- **Non-blocking Operations**: Keep UI responsive during GPS operations
- **Background Processing**: Handle GPS data in separate thread
- **Progressive Loading**: Show partial data while loading
- **Smooth Animations**: Use ImGui animations judiciously

## Testing Guidelines

### UI Testing
- **Resolution Testing**: Test at different screen resolutions
- **Window Resizing**: Verify proper layout at various window sizes
- **Docking Behavior**: Test window docking and undocking
- **Modal Dialogs**: Ensure proper modal behavior

### Usability Testing
- **New User Experience**: Test with someone unfamiliar with GPS
- **Common Tasks**: Time common workflows (connect, record, export)
- **Error Scenarios**: Test error handling and recovery
- **Hardware Testing**: Test with different GPS devices

### Accessibility Testing
- **Keyboard Only**: Navigate entire interface with keyboard
- **Screen Reader**: Test with screen reader software
- **High Contrast**: Test in high contrast mode
- **Color Blind**: Test with color blind simulation tools