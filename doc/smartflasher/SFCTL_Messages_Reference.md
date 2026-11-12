# SFCTL Messages Reference Guide

Complete reference guide for all SFCTL (SmartFlasher Control) protocol messages with correct NMEA checksums and detailed examples.

---

## Table of Contents

1. [Overview](#overview)
2. [Message Format](#message-format)
3. [Command Messages](#command-messages)
4. [Response Messages](#response-messages)
5. [Error Codes](#error-codes)
6. [Complete Message List](#complete-message-list)
7. [Checksum Calculation](#checksum-calculation)
8. [SFVAR - Runtime Variables](#sfvar---runtime-variables)

---

## Overview

SFCTL (SmartFlasher Control Protocol) is a custom NMEA-based protocol for configuring and controlling the SmartFlasher device. All messages follow the standard NMEA format:

```
$SFCTL,<command>[,param1][,param2][,...]*<checksum>
```

### Key Features
- **NMEA-compliant**: Standard NMEA format with XOR checksums
- **Bidirectional**: Commands from host, responses from device
- **Type-safe**: Structured message parsing and validation
- **Not bridged**: SFCTL messages are not forwarded to other devices
- **Statistics tracked**: All SFCTL messages are counted in frame statistics

---

## Message Format

### NMEA Structure
```
$ SFCTL , COMMAND , PARAM1 , PARAM2 * XX
│   │      │         │        │      │
│   │      │         │        │      └─ 2-digit hex checksum
│   │      │         │        └──────── Optional parameters (comma-separated)
│   │      │         └─────────────────── First parameter
│   │      └───────────────────────────── Command name
│   └──────────────────────────────────── Protocol identifier
└──────────────────────────────────────── Start delimiter
```

### Checksum Calculation
The checksum is calculated by XORing all characters between `$` and `*`, then formatting as 2-digit uppercase hexadecimal.

**Example:**
```
Message: SFCTL,STATUS
Checksum = 'S' ^ 'F' ^ 'C' ^ 'T' ^ 'L' ^ ',' ^ 'S' ^ 'T' ^ 'A' ^ 'T' ^ 'U' ^ 'S'
         = 0x53 ^ 0x46 ^ 0x43 ^ 0x54 ^ 0x4C ^ 0x2C ^ 0x53 ^ 0x54 ^ 0x41 ^ 0x54 ^ 0x55 ^ 0x53
         = 0x76
Result: $SFCTL,STATUS*76
```

---

## Command Messages

Commands are sent FROM the host/client TO the SmartFlasher device.

### 1. STATUS - Request System Status

**Purpose:** Request current system status including uptime, memory, and connection information.

**Format:**
```
$SFCTL,STATUS*76
```

**Example Response:**
```
$SFCTL,ACK,STATUS,COMPLETE*36
$SFCTL,STATUS,UPTIME,1234*XX
$SFCTL,STATUS,FRAMES,5678*XX
$SFCTL,STATUS,FPS,12*XX
$SFCTL,STATUS,FREE_HEAP,245678*XX
$SFCTL,STATUS,WIFI_CLIENTS,2*XX
$SFCTL,STATUS,FLASH_MODE,2*XX
```

**Response Fields:**
- `UPTIME` - System uptime in seconds
- `FRAMES` - Total frames processed since boot
- `FPS` - Current frames per second
- `FREE_HEAP` - Available heap memory in bytes
- `WIFI_CLIENTS` - Number of connected WiFi clients
- `FLASH_MODE` - Current flash mode (0-4)

---

### 2. STATS - Request Frame Statistics

**Purpose:** Request detailed statistics for all NMEA frame types processed.

**Format:**
```
$SFCTL,STATS*23
```

**Example Response:**
```
$SFCTL,ACK,STATS,COMPLETE*63
$SFCTL,STATS,PFLAA,1234*XX
$SFCTL,STATS,PFLAU,567*XX
$SFCTL,STATS,PFLAC,89*XX
$SFCTL,STATS,PFLAE,12*XX
$SFCTL,STATS,GPGGA,345*XX
$SFCTL,STATS,GPRMC,234*XX
$SFCTL,STATS,GPGSA,123*XX
$SFCTL,STATS,GPGSV,456*XX
$SFCTL,STATS,GPGLL,78*XX
$SFCTL,STATS,SFCTL,42*XX
$SFCTL,STATS,INVALID,5*XX
```

**Statistics Included:**
- **FLARM Messages**: PFLAA, PFLAU, PFLAC, PFLAE
- **GPS Messages**: GPGGA, GPRMC, GPGSA, GPGSV, GPGLL
- **Control Messages**: SFCTL
- **Invalid Messages**: INVALID (failed checksum or parse errors)

---

### 3. LIST - List All Parameters

**Purpose:** Request a list of all available configuration parameters.

**Format:**
```
$SFCTL,LIST*60
```

**Example Response:**
```
$SFCTL,ACK,LIST*05
$SFCTL,LIST,DEVICE_NAME,SmartFlasher*XX
$SFCTL,LIST,POWER_LEVEL,35*XX
$SFCTL,LIST,BT_ENABLED,true*XX
$SFCTL,LIST,WIFI_SSID,SmartFlasher-AP*XX
$SFCTL,LIST,BRIGHTNESS,3*XX
$SFCTL,LIST,FLASH_MODE,2*XX
```

Each LIST response includes the parameter name and its current value.

---

### 4. DEFAULTS - Reset to Factory Defaults

**Purpose:** Reset all configuration parameters to factory default values.

**Format:**
```
$SFCTL,DEFAULTS*7A
```

**Response:**
```
$SFCTL,ACK,DEFAULTS*1F
```

**Warning:** This command immediately resets all parameters to defaults and saves them to persistent storage. This action cannot be undone.

---

### 5. READ - Read Parameter Value

**Purpose:** Read the current value of a specific parameter.

**Format:**
```
$SFCTL,R,<parameter_name>*XX
```

**Examples:**
```
$SFCTL,R,DEVICE_NAME*5C
$SFCTL,R,POWER_LEVEL*4A
$SFCTL,R,BT_ENABLED*10
$SFCTL,R,BRIGHTNESS*3E
```

**Response (Success):**
```
$SFCTL,ACK,DEVICE_NAME,SmartFlasher*75
$SFCTL,ACK,POWER_LEVEL,35*7B
$SFCTL,ACK,BT_ENABLED,true*31
$SFCTL,ACK,BRIGHTNESS,3*79
```

**Response (Error):**
```
$SFCTL,NACK,UNKNOWN*01          // Parameter doesn't exist
```

---

### 6. WRITE - Write Parameter Value

**Purpose:** Write a new value to a specific parameter.

**Format:**
```
$SFCTL,W,<parameter_name>,<value>*XX
```

**Examples:**
```
$SFCTL,W,DEVICE_NAME,SmartFlasher*6B
$SFCTL,W,POWER_LEVEL,35*65
$SFCTL,W,POWER_LEVEL,40*67
$SFCTL,W,BT_ENABLED,true*2F
$SFCTL,W,BRIGHTNESS,3*75
```

**Response (Success):**
```
$SFCTL,ACK,DEVICE_NAME,SmartFlasher*75
$SFCTL,ACK,POWER_LEVEL,35*7B
```

**Response (Error):**
```
$SFCTL,NACK,UNKNOWN*01          // Parameter doesn't exist
$SFCTL,NACK,READ_ONLY*10        // Parameter is read-only
$SFCTL,NACK,INVALID_VALUE*0C    // Value is invalid for this parameter
```

**Notes:**
- Values are automatically validated based on parameter type
- Write operations are saved to persistent storage (NVS)
- Some parameters may require a reboot to take effect

---

### 7. RESET - System Reset

**Purpose:** Perform a complete system restart (ESP32 reboot).

**Format:**
```
$SFCTL,RESET*37
```

**Response:**
```
$SFCTL,ACK,RESET,RESTARTING*69
```

**Behavior:**
1. Device sends ACK response
2. 100ms delay to ensure message transmission
3. ESP32 performs hardware reset via `ESP.restart()`
4. Device reboots and reinitializes

**Warning:** All non-persistent state will be lost. Saved parameters are preserved.

---

### 8. FLASH - Flash Control Commands

**Purpose:** Control the LED flasher subsystem.

#### 8.1 FLASH ON - Enable Flashing

**Format:**
```
$SFCTL,FLASH,ON*1F
```

**Response:**
```
$SFCTL,ACK,FLASH,ON*78
```

**Behavior:** Enables LED flashing using the current flash mode setting.

---

#### 8.2 FLASH OFF - Disable Flashing

**Format:**
```
$SFCTL,FLASH,OFF*51
```

**Response:**
```
$SFCTL,ACK,FLASH,OFF*46
```

**Behavior:** Immediately disables all LED flashing.

---

#### 8.3 FLASH MODE - Set Flash Mode

**Purpose:** Set the flash pattern/urgency level.

**Format:**
```
$SFCTL,FLASH,MODE,<0-4>*XX
```

**Examples:**
```
$SFCTL,FLASH,MODE,0*01    // Mode 0: Off
$SFCTL,FLASH,MODE,1*00    // Mode 1: Low urgency
$SFCTL,FLASH,MODE,2*03    // Mode 2: Medium urgency
$SFCTL,FLASH,MODE,3*02    // Mode 3: High urgency
$SFCTL,FLASH,MODE,4*05    // Mode 4: Critical urgency
```

**Flash Mode Descriptions:**
- **Mode 0**: Flash disabled (same as FLASH,OFF)
- **Mode 1**: Single flash - Low threat
- **Mode 2**: Double flash - Medium threat
- **Mode 3**: Triple flash - High threat
- **Mode 4**: Continuous rapid flash - Critical threat

**Response:**
```
$SFCTL,ACK,FLASH_MODE,2*XX
```

---

#### 8.4 FLASH BRIGHTNESS - Set LED Brightness

**Purpose:** Set the LED brightness level.

**Format:**
```
$SFCTL,FLASH,BRIGHTNESS,<0-4>*XX
```

**Examples:**
```
$SFCTL,FLASH,BRIGHTNESS,0*0B    // Off (0%)
$SFCTL,FLASH,BRIGHTNESS,1*0A    // Low (25%)
$SFCTL,FLASH,BRIGHTNESS,2*09    // Medium (50%)
$SFCTL,FLASH,BRIGHTNESS,3*08    // High (75%)
$SFCTL,FLASH,BRIGHTNESS,4*0F    // Maximum (100%)
```

**Response:**
```
$SFCTL,ACK,FLASH_BRIGHTNESS,3*XX
```

**Note:** Actual brightness levels may vary based on hardware configuration.

---

#### 8.5 FLASH TEST - Test Flash Sequence

**Purpose:** Trigger a test flash sequence cycling through all modes.

**Format:**
```
$SFCTL,FLASH,TEST*08
```

**Response:**
```
$SFCTL,ACK,FLASH_TEST,STARTED*XX
```

**Behavior:**
- Cycles through all flash modes
- Useful for verifying LED functionality
- Test sequence runs for predetermined duration

---

#### 8.6 FLASH STATUS - Query Flash Status

**Purpose:** Request current flash subsystem status.

**Format:**
```
$SFCTL,FLASH,STATUS*0A
```

**Response:**
```
$SFCTL,ACK,FLASH_STATUS,COMPLETE*XX
$SFCTL,FLASH,ENABLED,true*XX
$SFCTL,FLASH,MODE,2*XX
$SFCTL,FLASH,BRIGHTNESS,3*XX
```

---

### 9. SYSINFO - System Information

**Purpose:** Request comprehensive system information including hardware, memory, software, and network details.

**Format:**
```
$SFCTL,SYSINFO*35
```

**Response:**
```
$SFCTL,ACK,SYSINFO,<system_info_string>*XX
```

**System Information Included:**

**Hardware:**
- Chip model (e.g., ESP32)
- Chip revision
- Number of CPU cores
- CPU frequency (MHz)
- MAC address
- Serial number
- Flash size and speed

**Memory:**
- Sketch size and free program space
- Heap memory (free/total)
- Minimum free heap (lowest point reached)
- File system usage

**Software:**
- SmartFlasher version
- Build timestamp
- ESP-IDF SDK version

**Network:**
- Access Point IP address
- Number of connected WiFi clients
- System uptime

**Example Response Data:**
```
========================================
       SYSTEM INFORMATION
========================================
Chip: ESP32 Rev 1
Cores: 2 @ 240 MHz
MAC: AA:BB:CC:DD:EE:FF
Serial: 2532001
Flash: 4.00 MB @ 40 MHz

Memory:
  Sketch: 1.70 MB / Free: 1.48 MB
  Heap: 239.98 KB / 320.00 KB
  FS: 64.00 KB / 256.00 KB

Software:
  SmartFlasher v1.2.0
  Build: 2026-03-29 10:30:45
  SDK: v4.4.6

Network:
  AP IP: 192.168.4.1
  Clients: 2
  Uptime: 1h 23m 45s
========================================
```

---

## Response Messages

Responses are sent FROM the SmartFlasher device TO the host/client.

### ACK - Acknowledgment (Success)

**Purpose:** Confirm successful command execution and return data.

**Format:**
```
$SFCTL,ACK,<command>[,value]*XX
```

**Examples:**
```
$SFCTL,ACK,LIST*05
$SFCTL,ACK,DEFAULTS*1F
$SFCTL,ACK,STATUS,COMPLETE*36
$SFCTL,ACK,DEVICE_NAME,SmartFlasher*75
$SFCTL,ACK,POWER_LEVEL,35*7B
$SFCTL,ACK,RESET,RESTARTING*69
$SFCTL,ACK,SYSINFO,OK*78
```

**Interpretation:**
- First field after ACK is the command/parameter name
- Optional second field contains the value or status message
- ACK indicates successful processing

---

### NACK - Negative Acknowledgment (Error)

**Purpose:** Report command execution failures with error codes.

**Format:**
```
$SFCTL,NACK,<error_code>*XX
```

**Examples:**
```
$SFCTL,NACK,UNKNOWN*01
$SFCTL,NACK,READ_ONLY*10
$SFCTL,NACK,INVALID_VALUE*0C
$SFCTL,NACK,TIMEOUT*12
$SFCTL,NACK,UNKNOWN_FLASH_ACTION*4F
```

---

## Error Codes

### Standard Error Codes

| Error Code | Description | Common Causes |
|------------|-------------|---------------|
| `UNKNOWN` | Command or parameter not recognized | Typo in command name, unsupported parameter |
| `READ_ONLY` | Attempt to write read-only parameter | System parameters like VOLTAGE, MAC address |
| `INVALID_VALUE` | Parameter value is invalid | Out of range, wrong type, format error |
| `TIMEOUT` | Operation timed out | Communication error, device busy |
| `UNKNOWN_FLASH_ACTION` | Invalid FLASH subcommand | Typo in FLASH action parameter |
| `PARSE_ERROR` | Failed to parse command | Malformed NMEA sentence |
| `BUSY` | Device is busy processing | Try again after current operation completes |

### Handling NACK Responses

When receiving a NACK:
1. Check the error code against the table above
2. Verify command syntax and parameters
3. For UNKNOWN errors, verify parameter name with LIST command
4. For INVALID_VALUE, check parameter type and range
5. For READ_ONLY, use READ instead of WRITE
6. Retry with corrected command if appropriate

---

## Complete Message List

### Query Commands (No Parameters)

| Command | Message | Checksum | Purpose |
|---------|---------|----------|---------|
| STATUS | `$SFCTL,STATUS*76` | 76 | System status |
| STATS | `$SFCTL,STATS*23` | 23 | Frame statistics |
| LIST | `$SFCTL,LIST*60` | 60 | All parameters |
| DEFAULTS | `$SFCTL,DEFAULTS*7A` | 7A | Reset to defaults |
| RESET | `$SFCTL,RESET*37` | 37 | System restart |
| SYSINFO | `$SFCTL,SYSINFO*35` | 35 | System information |

---

### Parameter Operations

#### READ Commands

| Parameter | Message | Checksum |
|-----------|---------|----------|
| DEVICE_NAME | `$SFCTL,R,DEVICE_NAME*5C` | 5C |
| POWER_LEVEL | `$SFCTL,R,POWER_LEVEL*4A` | 4A |
| BT_ENABLED | `$SFCTL,R,BT_ENABLED*10` | 10 |
| BRIGHTNESS | `$SFCTL,R,BRIGHTNESS*15` | 15 |
| FLASH_MODE | `$SFCTL,R,FLASH_MODE*10` | 10 |
| VOLTAGE | `$SFCTL,R,VOLTAGE*5E` | 5E |

#### WRITE Commands (Examples)

| Parameter | Value | Message | Checksum |
|-----------|-------|---------|----------|
| DEVICE_NAME | SmartFlasher | `$SFCTL,W,DEVICE_NAME,SmartFlasher*6B` | 6B |
| POWER_LEVEL | 35 | `$SFCTL,W,POWER_LEVEL,35*65` | 65 |
| POWER_LEVEL | 40 | `$SFCTL,W,POWER_LEVEL,40*67` | 67 |
| BT_ENABLED | true | `$SFCTL,W,BT_ENABLED,true*2F` | 2F |
| BT_ENABLED | false | `$SFCTL,W,BT_ENABLED,false*44` | 44 |
| BRIGHTNESS | 3 | `$SFCTL,W,BRIGHTNESS,3*0F` | 0F |

---

### Flash Control Commands

#### Basic Flash Control

| Command | Message | Checksum | Purpose |
|---------|---------|----------|---------|
| Flash ON | `$SFCTL,FLASH,ON*1F` | 1F | Enable flashing |
| Flash OFF | `$SFCTL,FLASH,OFF*51` | 51 | Disable flashing |
| Flash TEST | `$SFCTL,FLASH,TEST*08` | 08 | Test sequence |
| Flash STATUS | `$SFCTL,FLASH,STATUS*0A` | 0A | Query status |

#### Flash Mode Settings

| Mode | Message | Checksum | Description |
|------|---------|----------|-------------|
| 0 | `$SFCTL,FLASH,MODE,0*01` | 01 | Off |
| 1 | `$SFCTL,FLASH,MODE,1*00` | 00 | Low urgency |
| 2 | `$SFCTL,FLASH,MODE,2*03` | 03 | Medium urgency |
| 3 | `$SFCTL,FLASH,MODE,3*02` | 02 | High urgency |
| 4 | `$SFCTL,FLASH,MODE,4*05` | 05 | Critical urgency |

#### Brightness Settings

| Level | Message | Checksum | Description |
|-------|---------|----------|-------------|
| 0 | `$SFCTL,FLASH,BRIGHTNESS,0*0B` | 0B | Off (0%) |
| 1 | `$SFCTL,FLASH,BRIGHTNESS,1*0A` | 0A | Low (25%) |
| 2 | `$SFCTL,FLASH,BRIGHTNESS,2*09` | 09 | Medium (50%) |
| 3 | `$SFCTL,FLASH,BRIGHTNESS,3*08` | 08 | High (75%) |
| 4 | `$SFCTL,FLASH,BRIGHTNESS,4*0F` | 0F | Maximum (100%) |

---

### Response Messages

#### ACK Responses

| Response | Message | Checksum | Context |
|----------|---------|----------|---------|
| List ACK | `$SFCTL,ACK,LIST*05` | 05 | After LIST command |
| Defaults ACK | `$SFCTL,ACK,DEFAULTS*1F` | 1F | After DEFAULTS |
| Status ACK | `$SFCTL,ACK,STATUS,COMPLETE*36` | 36 | After STATUS |
| Stats ACK | `$SFCTL,ACK,STATS,COMPLETE*63` | 63 | After STATS |
| Reset ACK | `$SFCTL,ACK,RESET,RESTARTING*69` | 69 | Before restart |
| SysInfo ACK | `$SFCTL,ACK,SYSINFO,OK*78` | 78 | After SYSINFO |
| Read ACK | `$SFCTL,ACK,DEVICE_NAME,SmartFlasher*75` | 75 | Parameter read |
| Write ACK | `$SFCTL,ACK,POWER_LEVEL,35*7B` | 7B | Parameter written |

#### NACK Responses

| Error | Message | Checksum | Meaning |
|-------|---------|----------|---------|
| Unknown | `$SFCTL,NACK,UNKNOWN*01` | 01 | Unknown command/parameter |
| Read Only | `$SFCTL,NACK,READ_ONLY*10` | 10 | Cannot write parameter |
| Invalid Value | `$SFCTL,NACK,INVALID_VALUE*0C` | 0C | Value out of range/invalid |
| Timeout | `$SFCTL,NACK,TIMEOUT*12` | 12 | Operation timeout |
| Flash Action | `$SFCTL,NACK,UNKNOWN_FLASH_ACTION*4F` | 4F | Invalid FLASH subcommand |

---

## Checksum Calculation

### Algorithm

The NMEA checksum is calculated by:
1. Starting with checksum = 0
2. For each character between `$` and `*` (exclusive)
3. XOR the checksum with the character's ASCII value
4. Format the final result as 2-digit uppercase hexadecimal

### Example Code (Python)

```python
def calculate_nmea_checksum(sentence):
    """Calculate NMEA checksum (sentence without $ and *)"""
    checksum = 0
    for char in sentence:
        checksum ^= ord(char)
    return f"{checksum:02X}"

# Example
sentence = "SFCTL,STATUS"
checksum = calculate_nmea_checksum(sentence)
complete = f"${sentence}*{checksum}"
# Result: $SFCTL,STATUS*76
```

### Example Code (C++)

```cpp
String calculateChecksum(String sentence) {
    uint8_t checksum = 0;
    for (int i = 0; i < sentence.length(); i++) {
        checksum ^= sentence[i];
    }
    char hex[3];
    sprintf(hex, "%02X", checksum);
    return String(hex);
}

// Example
String sentence = "SFCTL,STATUS";
String checksum = calculateChecksum(sentence);
String complete = "$" + sentence + "*" + checksum;
// Result: $SFCTL,STATUS*76
```

### Manual Calculation Example

Calculate checksum for `SFCTL,RESET`:

```
S = 0x53
F = 0x46
C = 0x43
T = 0x54
L = 0x4C
, = 0x2C
R = 0x52
E = 0x45
S = 0x53
E = 0x45
T = 0x54

Checksum = 0x53 ^ 0x46 ^ 0x43 ^ 0x54 ^ 0x4C ^ 0x2C ^ 0x52 ^ 0x45 ^ 0x53 ^ 0x45 ^ 0x54
         = 0x37

Result: $SFCTL,RESET*37
```

---

## Usage Examples

### Basic Communication Flow

```
Host → Device: $SFCTL,STATUS*76
Device → Host: $SFCTL,ACK,STATUS,COMPLETE*36
Device → Host: $SFCTL,STATUS,UPTIME,1234*XX
Device → Host: $SFCTL,STATUS,FRAMES,5678*XX
Device → Host: $SFCTL,STATUS,FPS,12*XX
```

### Parameter Configuration

```
# Read current value
Host → Device: $SFCTL,R,POWER_LEVEL*4A
Device → Host: $SFCTL,ACK,POWER_LEVEL,35*7B

# Write new value
Host → Device: $SFCTL,W,POWER_LEVEL,40*67
Device → Host: $SFCTL,ACK,POWER_LEVEL,40*79

# Verify write
Host → Device: $SFCTL,R,POWER_LEVEL*4A
Device → Host: $SFCTL,ACK,POWER_LEVEL,40*79
```

### Error Handling

```
# Attempt to write read-only parameter
Host → Device: $SFCTL,W,VOLTAGE,12.5*XX
Device → Host: $SFCTL,NACK,READ_ONLY*10

# Invalid parameter name
Host → Device: $SFCTL,R,INVALID_PARAM*XX
Device → Host: $SFCTL,NACK,UNKNOWN*01

# Invalid value
Host → Device: $SFCTL,W,POWER_LEVEL,999*XX
Device → Host: $SFCTL,NACK,INVALID_VALUE*0C
```

### Flash Control Sequence

```
# Set brightness
Host → Device: $SFCTL,FLASH,BRIGHTNESS,3*08
Device → Host: $SFCTL,ACK,FLASH_BRIGHTNESS,3*XX

# Set mode
Host → Device: $SFCTL,FLASH,MODE,2*03
Device → Host: $SFCTL,ACK,FLASH_MODE,2*XX

# Enable flashing
Host → Device: $SFCTL,FLASH,ON*1F
Device → Host: $SFCTL,ACK,FLASH,ON*78
```

---

## Implementation Notes

### Message Processing

1. **Parsing**: NMEA sentences are parsed and validated
2. **Routing**: SFCTL messages are routed to `SFCTLHandler`
3. **Execution**: Command handlers execute requested operations
4. **Response**: ACK/NACK responses are generated and sent
5. **Statistics**: All SFCTL messages are counted in frame statistics

### File Locations

- **Protocol Definition**: `lib/SFProto/src/NMEAStructures.h`
- **Message Parsing**: `lib/SFProto/src/NMEAStructures.cpp`
- **Command Handler**: `lib/SFProto/src/SFCTLHandler.cpp`
- **Message Generation**: `lib/SFProto/src/NMEAGenerator.cpp`

### Adding New Commands

To add a new SFCTL command:

1. Add enum value to `SFCTLType` in `NMEAStructures.h`
2. Add parsing logic in `SFCTL::parse()` in `NMEAStructures.cpp`
3. Add handler method declaration in `SFCTLHandler.h`
4. Add case to switch statement in `SFCTLHandler::handleFrame()`
5. Implement handler method in `SFCTLHandler.cpp`
6. Update this documentation with the new command

---

## Testing

### Test Commands File

A complete set of test commands with correct checksums is available at:
`testjig/SampleData/sfctl_commands.txt`

### Testing Tools

- **Serial Monitor**: Send commands directly via serial port
- **Web Interface**: Use built-in web interface for testing
- **Python Script**: Use provided scripts for automated testing
- **Test Jig**: Hardware test jig with predefined test sequences

### Checksum Verification

Always verify checksums before sending commands. Incorrect checksums will cause the device to reject the message and increment the INVALID message counter.

---

## Best Practices

1. **Always include checksums**: Messages without valid checksums are rejected
2. **Wait for ACK/NACK**: Ensure previous command completed before sending next
3. **Handle errors gracefully**: Check for NACK responses and retry if appropriate
4. **Use LIST command**: Query available parameters before READ/WRITE operations
5. **Validate values**: Ensure parameter values are within valid range
6. **Save important data**: Use WRITE to persist configuration changes
7. **Test thoroughly**: Use FLASH,TEST to verify hardware before deployment

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-29 | Initial release with all SFCTL commands |
| 1.1 | 2026-03-29 | Added SYSINFO command documentation |

---

## Support

For questions, issues, or contributions:
- **Project**: SmartFlasher
- **Documentation**: `/docs/`
- **Examples**: `/testjig/SampleData/`
- **Source Code**: `/lib/SFProto/`

---

*End of SFCTL Messages Reference Guide*

---

## SFVAR - Runtime Variables

`$SFVAR` is a separate sentence for reading **runtime variables** - values that are not configuration parameters and change quickly (ADC temperature and voltage, uptime, flight state...). Variables are read-only, not stored in NVS and evaluated at the moment of the query. Full description: [SFVAR_Variables.md](SFVAR_Variables.md).

| Request | Response |
|---------|----------|
| `$SFVAR,TEMP*70` | `$SFVAR,TEMP,23.4*47` |
| `$SFVAR,TEMP,VOLTAGE*1E` | `$SFVAR,TEMP,23.4,VOLTAGE,12.31*2A` |
| `$SFVAR*50` | one `$SFVAR,<NAME>,<VALUE>` per variable |
| `$SFVAR,FOO*3A` | `$SFVAR,NACK,UNKNOWN_VAR,FOO*6F` |
