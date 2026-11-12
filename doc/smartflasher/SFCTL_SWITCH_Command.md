# SFCTL SWITCH Command Documentation

## Overview

The SFCTL SWITCH command provides control over various hardware outputs on the SmartFlasher device, including power ports and LED indicators. This command allows remote control of hardware states for testing and operational purposes.

## Command Format

```
$SFCTL,SWITCH,<target>,<state>*CS
```

Where:
- `target`: The hardware target to control
- `state`: The desired state (ON or OFF)
- `*CS`: NMEA checksum (automatically calculated)

## Supported Targets

| Target | Description | Type |
|--------|-------------|------|
| `FLARM_PORT_POWER` | FLARM port power control | ezLED |
| `DISPLAY_PORT_POWER` | Display port power control | ezLED |
| `POWER_LED` | Power LED indicator | Led |
| `EXT_LED` | External LED indicator | Led |
| `BT_LED` | Bluetooth LED indicator | Led |

## Commands

### 1. Turn FLARM Port Power ON

**Command:**
```
$SFCTL,SWITCH,FLARM_PORT_POWER,ON*63
```

**Response:**
```
$SFCTL,ACK,SWITCH,FLARM_PORT_POWER,ON*06
```

**Description:** Enables power to the FLARM serial port.

---

### 2. Turn FLARM Port Power OFF

**Command:**
```
$SFCTL,SWITCH,FLARM_PORT_POWER,OFF*2D
```

**Response:**
```
$SFCTL,ACK,SWITCH,FLARM_PORT_POWER,OFF*48
```

**Description:** Disables power to the FLARM serial port.

---

### 3. Turn Display Port Power ON

**Command:**
```
$SFCTL,SWITCH,DISPLAY_PORT_POWER,ON*6D
```

**Response:**
```
$SFCTL,ACK,SWITCH,DISPLAY_PORT_POWER,ON*08
```

**Description:** Enables power to the display serial port.

---

### 4. Turn Display Port Power OFF

**Command:**
```
$SFCTL,SWITCH,DISPLAY_PORT_POWER,OFF*23
```

**Response:**
```
$SFCTL,ACK,SWITCH,DISPLAY_PORT_POWER,OFF*46
```

**Description:** Disables power to the display serial port.

---

### 5. Turn Power LED ON

**Command:**
```
$SFCTL,SWITCH,POWER_LED,ON*3C
```

**Response:**
```
$SFCTL,ACK,SWITCH,POWER_LED,ON*59
```

**Description:** Turns on the power status LED continuously.

---

### 6. Turn Power LED OFF

**Command:**
```
$SFCTL,SWITCH,POWER_LED,OFF*72
```

**Response:**
```
$SFCTL,ACK,SWITCH,POWER_LED,OFF*17
```

**Description:** Turns off the power status LED.

---

### 7. Turn External LED ON

**Command:**
```
$SFCTL,SWITCH,EXT_LED,ON*2A
```

**Response:**
```
$SFCTL,ACK,SWITCH,EXT_LED,ON*4F
```

**Description:** Turns on the external LED indicator.

---

### 8. Turn External LED OFF

**Command:**
```
$SFCTL,SWITCH,EXT_LED,OFF*64
```

**Response:**
```
$SFCTL,ACK,SWITCH,EXT_LED,OFF*01
```

**Description:** Turns off the external LED indicator.

---

### 9. Turn Bluetooth LED ON

**Command:**
```
$SFCTL,SWITCH,BT_LED,ON*75
```

**Response:**
```
$SFCTL,ACK,SWITCH,BT_LED,ON*10
```

**Description:** Turns on the Bluetooth status LED.

---

### 10. Turn Bluetooth LED OFF

**Command:**
```
$SFCTL,SWITCH,BT_LED,OFF*3B
```

**Response:**
```
$SFCTL,ACK,SWITCH,BT_LED,OFF*5E
```

**Description:** Turns off the Bluetooth status LED.

---

## Error Responses

### Missing Parameters

**Command:**
```
$SFCTL,SWITCH*41
```

**Response:**
```
$SFCTL,NACK,MISSING_PARAMS*6B
```

**Description:** Returned when target or state is not provided.

---

### Unknown Target

**Command:**
```
$SFCTL,SWITCH,INVALID_TARGET,ON*7D
```

**Response:**
```
$SFCTL,NACK,UNKNOWN_TARGET*05
```

**Description:** Returned when the specified target is not recognized.

---

## Usage Examples

### Example 1: Enable FLARM Port for Testing

```bash
# Turn on FLARM port power
$SFCTL,SWITCH,FLARM_PORT_POWER,ON*63

# Response: $SFCTL,ACK,SWITCH,FLARM_PORT_POWER,ON*06
```

---

### Example 2: Disable Display Port

```bash
# Turn off display port power
$SFCTL,SWITCH,DISPLAY_PORT_POWER,OFF*23

# Response: $SFCTL,ACK,SWITCH,DISPLAY_PORT_POWER,OFF*46
```

---

### Example 3: LED Status Indication

```bash
# Turn on power LED
$SFCTL,SWITCH,POWER_LED,ON*3C

# Turn on external LED
$SFCTL,SWITCH,EXT_LED,ON*2A

# Both LEDs are now on
```

---

### Example 4: Complete Port Setup

```bash
# 1. Enable both ports
$SFCTL,SWITCH,FLARM_PORT_POWER,ON*63
$SFCTL,SWITCH,DISPLAY_PORT_POWER,ON*6D

# 2. Turn on status LEDs
$SFCTL,SWITCH,POWER_LED,ON*3C
$SFCTL,SWITCH,BT_LED,ON*75

# 3. Configuration complete
```

---

### Example 5: Power Down Sequence

```bash
# 1. Turn off external LED
$SFCTL,SWITCH,EXT_LED,OFF*64

# 2. Turn off display port
$SFCTL,SWITCH,DISPLAY_PORT_POWER,OFF*23

# 3. Turn off FLARM port
$SFCTL,SWITCH,FLARM_PORT_POWER,OFF*2D

# 4. Turn off power LED
$SFCTL,SWITCH,POWER_LED,OFF*72
```

---

## Error Codes

| Error Code | Description |
|------------|-------------|
| `MISSING_PARAMS` | Target or state parameter not provided |
| `UNKNOWN_TARGET` | The specified target is not recognized |

---

## Implementation Notes

1. **Case Insensitivity**: Target and state values are converted to uppercase internally, so `on`, `ON`, and `On` are all valid.

2. **Hardware Types**: The command supports two types of hardware:
   - `ezLED`: LED control using the ezLED library (supports turnOn/turnOff)
   - `Led`: LED control using the custom Led class

3. **Safety**: The command provides direct hardware access - use with caution in production environments.

4. **No Persistence**: State changes made via SWITCH command are not persisted to EEPROM. Use parameter-based control for persistent settings.

5. **Integration**: The handler is registered via `registerSFCTLRoute` in the SFCTLHandler constructor.

---

## Code Implementation

### Enum Definition (NMEAStructures.h)

```cpp
enum SFCTLType {
    SFCTL_ACK = 1,
    SFCTL_NACK = 2,
    SFCTL_LIST = 3,
    SFCTL_DEFAULTS = 4,
    SFCTL_READ = 5,
    SFCTL_WRITE = 6,
    SFCTL_STATUS = 7,
    SFCTL_STATS = 8,
    SFCTL_FLASH = 9,
    SFCTL_RESET = 10,
    SFCTL_SYSINFO = 11,
    SFCTL_SWITCH = 12,    // <-- New command
    SFCTL_UNKNOWN = 0
};
```

### Handler Declaration (SFCTLHandler.h)

```cpp
class SFCTLHandler : public SFHandler
{
public:
    // ... other methods ...
    void handleSwitch(const SFCTL &msg);
};
```

### Handler Implementation (SFCTLHandler.cpp)

```cpp
void SFCTLHandler::handleSwitch(const SFCTL &msg)
{
    log_i("SWITCH command received");
    
    std::vector<String> fields = proto->split(msg.value);
    
    if (fields.size() < 2)
    {
        proto->nack("MISSING_PARAMS");
        return;
    }
    
    String what = fields[0];
    String state = fields[1];
    what.toUpperCase();
    state.toUpperCase();
    
    bool success = false;
    
    if (what == "FLARM_PORT_POWER")
    {
        if (state == "ON") {
            sf->getFlarmPortPower().turnOn();
            success = true;
        } else if (state == "OFF") {
            sf->getFlarmPortPower().turnOff();
            success = true;
        }
    }
    else if (what == "DISPLAY_PORT_POWER")
    {
        // Similar handling...
    }
    // ... other targets ...
    
    if (success)
    {
        proto->sendNMEA(gen.generateSFCTL_ACK("SWITCH", what + "," + state));
    }
}
```

---

## See Also

- [SFCTL Protocol Documentation](./SFCTL_Protocol.md)
- [SFCTL FLASH Command](./SFCTL_FLASH_Command.md)
- [NMEAStructures.h](../../lib/SFProto/src/NMEAStructures.h)
- [SFCTLHandler.h](../../lib/SFProto/src/SFCTLHandler.h)
- [SFCTLHandler.cpp](../../lib/SFProto/src/SFCTLHandler.cpp)

---

**Version:** 1.0.0
**Last Updated:** 2026-04-02
