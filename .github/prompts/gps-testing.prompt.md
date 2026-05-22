# GPS Hardware & Testing Prompt

## Context
You are working with GPS hardware integration for the GPC (GPS Console) application.

## Supported GPS Hardware

### USB GPS Devices
- **MediaTek-based modules** (MT3333, MT3339, etc.)
- **u-blox modules** (NEO series)
- **SiRF-based devices**
- **Generic NMEA 0183 compatible GPS**

### Connection Types
- **USB to Serial**: Most common (FTDI, CP2102, CH340)
- **Direct USB**: GPS modules with built-in USB interface
- **Bluetooth**: Future support planned

## Port Detection

### macOS Ports
```bash
# Check available ports
ls /dev/cu.usbmodem*
ls /dev/tty.usbmodem*
ls /dev/cu.usbserial*

# Common patterns
/dev/cu.usbmodem14201     # Built-in USB GPS
/dev/cu.usbserial-A50285B # FTDI adapter
```

### Linux Ports
```bash
# Check available ports
ls /dev/ttyUSB*
ls /dev/ttyACM*

# Common patterns
/dev/ttyUSB0              # USB-to-serial adapter
/dev/ttyACM0              # CDC ACM device
```

## NMEA Protocol Support

### Standard Sentences
- **RMC**: Recommended Minimum GPS data (position, time, speed)
- **GGA**: GPS Fix Data (position, satellites, altitude)
- **GSA**: GPS DOP and active satellites
- **GSV**: GPS Satellites in view (detailed satellite info)
- **GLL**: Geographic position (lat/lon)
- **VTG**: Track made good and ground speed
- **ZDA**: Time and date

### MediaTek (MTK) Commands
```
$PMTK605*31       # Query firmware version
$PMTK104*37       # Cold restart
$PMTK101*32       # Hot restart
$PMTK102*31       # Warm restart
$PMTK220,1000*1F  # Set update rate to 1Hz
$PMTK220,200*2C   # Set update rate to 5Hz
$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28  # RMC+GGA only
$PMTK314,-1*04    # Restore default NMEA sentences
```

## Testing Procedures

### Connection Testing
1. **Physical Connection**
   - Verify USB cable quality
   - Check GPS antenna connection
   - Ensure proper power supply

2. **Port Detection**
   - Use GPC Connection Dialog
   - Verify port appears in system
   - Test with external terminal program

3. **Baud Rate Testing**
   - Try common rates: 9600, 4800, 38400
   - Most GPS devices default to 9600
   - Some modules use 38400 or 115200

### NMEA Data Validation
1. **Raw Data Check**
   - Open Raw Data Console tab
   - Verify NMEA sentences appearing
   - Check sentence types and frequency

2. **Data Parsing**
   - Monitor Telemetry panel for updates
   - Verify coordinates appear reasonable
   - Check satellite count and signal strength

3. **Fix Quality**
   - Move GPS to clear sky view
   - Wait for satellite acquisition (cold start: 30s-2min)
   - Verify fix status changes to "Valid"

### Feature Testing

### Map & Tracking
1. **Position Display**
   - Verify position appears on map
   - Check coordinate accuracy
   - Test zoom/pan functionality

2. **Track Recording**
   - Start recording via Tools menu
   - Move GPS device to create track
   - Verify track points appear on map

3. **GPX Export**
   - Record some track data
   - Export via Tools → Export GPX
   - Verify file created in data/ directory

### Satellite Monitoring
1. **Sky Plot**
   - Check satellite positions appear
   - Verify signal strength color coding
   - Test satellite selection/details

2. **Satellite List**
   - Verify PRN numbers appear
   - Check elevation/azimuth values
   - Monitor SNR (signal-to-noise) ratios

### Satellite Monitoring - Detailed Test Matrix

#### Test Case S-01: Basic Satellite Visibility
- **Precondition**: GPS connected, raw NMEA stream active (GSV sentences visible)
- **Steps**:
   1. Open **Satellites** panel and **Sky Plot** panel
   2. Wait 10-30 seconds for updates
   3. Compare visible satellite count in Telemetry vs list rows
- **Expected**:
   - Satellite list is not empty
   - PRN values are shown (non-zero)
   - Sky Plot has at least one rendered satellite marker

#### Test Case S-02: SNR Quality Rendering
- **Precondition**: Device has satellite lock or partial lock
- **Steps**:
   1. Observe SNR bars in **Satellite List**
   2. Check color transitions while signal changes
- **Expected**:
   - SNR >~35 appears as strong (green)
   - SNR between ~20-35 appears medium (yellow)
   - SNR <~20 appears weak (red/orange depending on panel)
   - No SNR should be shown as empty/placeholder (e.g. `--`)

#### Test Case S-03: Sky Plot Geometry Consistency
- **Precondition**: GSV provides azimuth/elevation
- **Steps**:
   1. Pick 2-3 satellites from list (known azimuth/elevation)
   2. Cross-check approximate direction on Sky Plot
   3. Repeat after 30-60 seconds
- **Expected**:
   - Higher elevation satellites render closer to center
   - Lower elevation satellites render near outer rings
   - East/West/North/South orientation remains consistent

#### Test Case S-04: Used-in-Fix Correlation
- **Precondition**: Valid position fix available
- **Steps**:
   1. Observe satellites marked as used in fix (GSA context)
   2. Compare with visual highlight in Sky Plot
- **Expected**:
   - Satellites used in fix are clearly distinguishable
   - Used count in telemetry aligns with visible fix indicators

#### Test Case S-05: Degraded Signal Behavior
- **Precondition**: Start outdoors with stable fix
- **Steps**:
   1. Move to obstructed environment (near building/indoors)
   2. Observe SNR decline and satellite visibility changes for 1-2 min
- **Expected**:
   - Visible satellites and SNR may drop gracefully
   - UI remains responsive (no freeze/crash)
   - Fix quality transitions are reflected in Telemetry

#### Suggested Pass Criteria (Satellite Module)
- No crash/freeze during continuous updates for 5+ minutes
- Satellite counts remain internally consistent across panels
- SNR visualization and Sky Plot positions are coherent with NMEA input
- Used-in-fix indicators behave consistently with fix state

### Compass & Direction
1. **Heading Display**
   - Move GPS device while walking/driving
   - Verify compass arrow follows movement
   - Check speed threshold for heading updates

2. **Magnetic Declination**
   - Test manual declination adjustment
   - Verify true heading calculation
   - Compare with known magnetic declination

## Common GPS Issues

### No Satellite Fix
- **Cold Start**: Wait 30s-2min for initial fix
- **Location**: Move to open area, away from buildings
- **Antenna**: Check antenna connection and positioning
- **Time**: GPS needs current almanac data

### Poor Accuracy
- **Multipath**: Avoid reflective surfaces (buildings, water)
- **Interference**: Check for 2.4GHz devices nearby
- **Dilution**: Monitor DOP values in GSA sentences
- **Satellite Count**: Need 4+ satellites for 3D fix

### No NMEA Data
- **Baud Rate**: Try different rates (9600, 4800, 38400)
- **Cable**: Test with known working cable
- **Driver**: Verify USB-to-serial driver installed
- **Power**: Ensure GPS module powered properly

## GPS Hardware Recommendations

### Development/Testing
- **Adafruit Ultimate GPS** (MTK3339-based)
- **u-blox NEO-8M/NEO-9M** modules
- **USB GPS dongles** (for easy testing)

### Production Use
- **High-sensitivity modules** for weak signal areas
- **External antennas** for vehicle installation
- **Fast acquisition** modules for mobile use

## Performance Expectations

### Update Rates
- **1Hz**: Standard for most applications
- **5Hz**: Good for vehicle tracking
- **10Hz**: High-precision applications
- **18Hz**: Maximum for some modules

### Accuracy
- **Standard GPS**: 3-5 meter accuracy
- **DGPS/WAAS**: 1-3 meter accuracy
- **RTK GPS**: Centimeter accuracy (advanced)

### Acquisition Times
- **Hot Start**: 1-15 seconds (recent almanac)
- **Warm Start**: 15-45 seconds (partial data)
- **Cold Start**: 30s-2min (no prior data)