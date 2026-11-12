# SFCTL FLASH Command Documentation

## Overview

The SFCTL FLASH command provides control over the LedFlasher module, which manages LED flashing on GPIO 25 with DAC-controlled brightness levels and multiple flashing modes.

## Command Format

```
$SFCTL,FLASH,<action>[,<param1>[,<param2>]]*CS
```

Where:
- `action`: The operation to perform (ON, OFF, MODE, BRIGHTNESS, TEST, STATUS)
- `param1`: Optional first parameter (depends on action)
- `param2`: Optional second parameter (depends on action)
- `*CS`: Checksum (automatically calculated)

## Actions

### 1. ON - Turn LED Flasher On

**Command:**
```
$SFCTL,FLASH,ON*1F
```

**Response:**
```
$SFCTL,ACK,FLASH,ON*7A
```

**Description:** Enables the LED flasher. If a mode is set, it will start flashing according to that mode. If MODE_OFF is set, the LED will turn on continuously.

---

### 2. OFF - Turn LED Flasher Off

**Command:**
```
$SFCTL,FLASH,OFF*51
```

**Response:**
```
$SFCTL,ACK,FLASH,OFF*34
```

**Description:** Disables the LED flasher and turns off the LED.

---

### 3. MODE - Set Flash Mode

**Command:**
```
$SFCTL,FLASH,MODE,<mode_number>*XX
```

**Parameters:**
- `mode_number`: Integer 0-4

**Response:**
```
$SFCTL,ACK,FLASH_MODE,<mode_number>*XX
```

**Flash Modes:**

| Mode | Sequence | Description |
|------|----------|-------------|
| 0 | OFF | No flashing (LED stays on if flasher is ON) |
| 1 | flash 50ms, break 100ms, flash 50ms, break 2000ms | Two quick flashes, long pause |
| 2 | flash 100ms, break 100ms, flash 100ms, break 1500ms | Two medium flashes, medium pause |
| 3 | flash 200ms, break 100ms, flash 200ms, break 1200ms | Two long flashes, short pause |
| 4 | flash 100ms, break 80ms, flash 100ms, break 80ms, flash 100ms, break 1000ms | Three quick flashes, short pause |

**All modes loop continuously.**

**Example:**
```
$SFCTL,FLASH,MODE,1*00
```

**Error Response:**
```
$SFCTL,NACK,MISSING_MODE*51    # No mode number provided
$SFCTL,NACK,INVALID_MODE*44    # Mode number out of range
```

---

### 4. BRIGHTNESS - Set LED Brightness

**Command:**
```
$SFCTL,FLASH,BRIGHTNESS,<level>*XX
```

**Parameters:**
- `level`: Integer 0-5

**Response:**
```
$SFCTL,ACK,FLASH_BRIGHTNESS,<level>*XX
```

**Brightness Levels (DAC Voltage Control):**

| Level | Voltage | DAC Value | Description |
|-------|---------|-----------|-------------|
| 0 | 20% | 51/255 | Very dim |
| 1 | 27% | 69/255 | Dim |
| 2 | 35% | 89/255 | Medium-dim |
| 3 | 40% | 102/255 | Medium (default) |
| 4 | 45% | 115/255 | Medium-bright |
| 5 | 100% | 255/255 | Maximum brightness |

**Example:**
```
$SFCTL,FLASH,BRIGHTNESS,5*0E    # Set to 100% brightness
```

**Error Response:**
```
$SFCTL,NACK,MISSING_BRIGHTNESS*5B    # No brightness level provided
$SFCTL,NACK,INVALID_BRIGHTNESS*4E    # Brightness level out of range
```

---

### 5. TEST - Execute Test Flash

**Command:**
```
$SFCTL,FLASH,TEST*08
```

**Response:**
```
$SFCTL,ACK,FLASH_TEST,STARTED*73
```

**Description:** Executes a test flash sequence that consists of 5 flashes (200ms on, 100ms off). The test flash pauses the current mode and automatically resumes it after completion.

**Test Sequence:**
```
flash 200ms, break 100ms,
flash 200ms, break 100ms,
flash 200ms, break 100ms,
flash 200ms, break 100ms,
flash 200ms
```

**Note:** Test flash can be triggered at any time, even when the flasher is off. It will temporarily turn on the LED for the test, then return to the previous state.

**Error Response:**
```
$SFCTL,NACK,TEST_ALREADY_ACTIVE*15    # Test flash is already running
```

---

### 6. STATUS - Query Flash Status

**Command:**
```
$SFCTL,FLASH,STATUS*0A
```

**Response:** Multiple STATUS messages followed by ACK
```
$SFCTL,STATUS,FLASH_ON,<0|1>*XX
$SFCTL,STATUS,FLASH_MODE,<0-4>*XX
$SFCTL,STATUS,FLASH_BRIGHTNESS,<0-4>*XX
$SFCTL,STATUS,FLASH_TEST,<0|1>*XX
$SFCTL,ACK,FLASH_STATUS,COMPLETE*39
```

**Status Fields:**
- `FLASH_ON`: 1 if flasher is on, 0 if off
- `FLASH_MODE`: Current mode (0-4)
- `FLASH_BRIGHTNESS`: Current brightness level (0-5)
- `FLASH_TEST`: 1 if test flash is active, 0 if not

---

## Usage Examples

### Example 1: Basic LED Flashing
```bash
# Turn on flasher
$SFCTL,FLASH,ON*1F

# Set mode 1 (two quick flashes)
$SFCTL,FLASH,MODE,1*00

# Set brightness to 40%
$SFCTL,FLASH,BRIGHTNESS,3*08
```

### Example 2: Change Mode While Running
```bash
# Currently in mode 1, switch to mode 3
$SFCTL,FLASH,MODE,3*02

# Response: $SFCTL,ACK,FLASH_MODE,3*14
```

### Example 3: Brightness Adjustment
```bash
# Increase brightness to maximum
$SFCTL,FLASH,BRIGHTNESS,5*0E

# Decrease brightness to minimum
$SFCTL,FLASH,BRIGHTNESS,0*0B
```

### Example 4: Test Flash
```bash
# Run test flash (works even when flasher is off)
$SFCTL,FLASH,TEST*08

# Response: $SFCTL,ACK,FLASH_TEST,STARTED*73
# LED will flash 5 times, then resume previous state
```

### Example 5: Query Status
```bash
# Get current flash status
$SFCTL,FLASH,STATUS*0A

# Response:
# $SFCTL,STATUS,FLASH_ON,1*49
# $SFCTL,STATUS,FLASH_MODE,1*4B
# $SFCTL,STATUS,FLASH_BRIGHTNESS,3*43
# $SFCTL,STATUS,FLASH_TEST,0*5F
# $SFCTL,ACK,FLASH_STATUS,COMPLETE*39
```

### Example 6: Turn Off
```bash
# Turn off flasher
$SFCTL,FLASH,OFF*51

# Response: $SFCTL,ACK,FLASH,OFF*34
```

---

## Error Codes

| Error Code | Description |
|------------|-------------|
| INVALID_FLASH_CMD | Missing action parameter |
| MISSING_MODE | MODE command without mode number |
| INVALID_MODE | Mode number out of range (must be 0-4) |
| MISSING_BRIGHTNESS | BRIGHTNESS command without brightness level |
| INVALID_BRIGHTNESS | Brightness level out of range (must be 0-5) |
| TEST_ALREADY_ACTIVE | Test flash is already running |
| UNKNOWN_FLASH_ACTION | Unrecognized action command |

---

## Implementation Notes

1. **Integration**: The handler must be added to the `registerSFCTLRoute` callback in `main.cpp`
2. **Loop Requirement**: `ledFlasher.loop()` must be called in the main loop for flashing to work
3. **GPIO**: Default GPIO is 25 (can be changed in LedFlasher constructor)
4. **DAC Control**: Uses ESP32 DAC for analog brightness control
5. **ezLED Library**: Requires the ezLED library for LED control
6. **Test Flash Priority**: Test flash pauses the current mode and resumes it after completion
7. **State Persistence**: Mode and brightness settings are preserved when turning on/off

---

## Complete Workflow Example

```bash
# 1. Initialize and configure
$SFCTL,FLASH,BRIGHTNESS,3*08      # Set 40% brightness
$SFCTL,FLASH,MODE,2*03             # Set mode 2
$SFCTL,FLASH,ON*1F                 # Turn on

# 2. LED is now flashing in mode 2 at 40% brightness

# 3. Run test
$SFCTL,FLASH,TEST*08               # Test flash (pauses mode 2)
# Test completes, mode 2 resumes automatically

# 4. Change mode
$SFCTL,FLASH,MODE,4*05             # Switch to mode 4 (3 flashes)

# 5. Adjust brightness
$SFCTL,FLASH,BRIGHTNESS,5*0E       # Increase to 100%

# 6. Check status
$SFCTL,FLASH,STATUS*0A             # Query current state

# 7. Turn off
$SFCTL,FLASH,OFF*51                # Turn off flasher
```

---

## See Also

- `LedFlasher.h` - LED flasher class interface
- `LedFlasher.cpp` - LED flasher implementation
- `examples/led_flasher_example.cpp` - Standalone usage example
- `examples/sfctl_flash_handler.cpp` - SFCTL integration example
- `NMEAStructures.h` - SFCTL_FLASH type definition
- `NMEAGenerator.h` - generateSFCTL_FLASH() method
