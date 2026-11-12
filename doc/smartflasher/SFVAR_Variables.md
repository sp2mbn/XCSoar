# SFVAR - Runtime Variables

`$SFVAR` reads **runtime variables**: values that are not configuration parameters, e.g. quickly changing ADC measurements (temperature, supply voltage), uptime or flight state.

| | Parameters (`$SFCTL,R` / `$SFCTL,W`) | Variables (`$SFVAR`) |
|---|---|---|
| Source | `ParameterManager` (NVS) | `VariableRegistry` (getter functions) |
| Writable | per parameter | never (read-only) |
| Persisted | yes | no - computed at query time |

---

## Sentence Format

### Request (host → SmartFlasher)

```
$SFVAR[,<NAME1>[,<NAME2>...]]*<checksum>
```

- Names are case-insensitive (`temp` = `TEMP`).
- Any number of names can be requested in one sentence (mind the NMEA line length - max. 128 bytes on client ports).
- No names = read all variables.

### Response (SmartFlasher → host)

```
$SFVAR,<NAME1>,<VALUE1>[,<NAME2>,<VALUE2>...]*<checksum>
```

The response repeats each requested name followed by its value, in the request order.

- Request without names: one `$SFVAR,<NAME>,<VALUE>` sentence per registered variable.
- Unknown name: `$SFVAR,NACK,UNKNOWN_VAR,<NAME>` (no values are returned for that request).

### Routing

- Works on every input port: USB, display port, Bluetooth Classic and BLE.
- The response goes back to the port the request arrived on.
- `$SFVAR` is **never forwarded** to the FLARM (client ports filter it like `$SFCTL`).

---

## Examples

| Request | Response |
|---------|----------|
| `$SFVAR,TEMP*70` | `$SFVAR,TEMP,23.4*47` |
| `$SFVAR,VOLTAGE*3E` | `$SFVAR,VOLTAGE,12.31*3D` |
| `$SFVAR,TEMP,VOLTAGE*1E` | `$SFVAR,TEMP,23.4,VOLTAGE,12.31*2A` |
| `$SFVAR*50` | `$SFVAR,TEMP,23.4*47`<br>`$SFVAR,TEMP_RAW,...`<br>...one per variable |
| `$SFVAR,FOO*3A` | `$SFVAR,NACK,UNKNOWN_VAR,FOO*6F` |

Checksum: XOR of all characters between `$` and `*` (see [SFCTL_Messages_Reference.md](SFCTL_Messages_Reference.md#checksum-calculation)).

---

## Available Variables

| Name | Unit | Format | Description |
|------|------|--------|-------------|
| `TEMP` | °C | 1 decimal | Temperature (ADC, GPIO34). `-1000.0` = read error |
| `TEMP_RAW` | - | integer 0-4095 | Raw ADC value of the temperature channel |
| `TEMP_MV` | mV | integer | Calibrated temperature channel voltage |
| `VOLTAGE` | V | 2 decimals | Supply voltage (ADC, GPIO35, divider applied) |
| `VOLTAGE_RAW` | - | integer 0-4095 | Raw ADC value of the voltage channel |
| `VOLTAGE_MV` | mV | integer | Calibrated voltage channel voltage (before divider) |
| `UPTIME` | s | integer | Time since boot |
| `HEAP` | B | integer | Free heap |
| `AIRBORNE` | - | 0/1 | Flight state |
| `GPS_FIX` | - | 0/1 | GPS fix from FLARM |
| `SW_VERSION` | - | text | Firmware version (`SystemInfo::getSoftwareVersion()`), e.g. `0.9.106` |
| `BUILD_TIMESTAMP` | - | text | Firmware build time (`SystemInfo::getBuildTimestamp()`), format `YYYY-MM-DD HH:MM`, e.g. `2026-05-06 01:28` |
| `FLARM_CONNECTED` | - | 0/1 | FLARM data received |

ADC values use the `ADCMeasurement` cache, so frequent polling does not trigger an ADC read each time.

---

## Adding a Variable

Variables are registered in `SmartFlasher::registerVariables()` (`src/SmartFlasher.cpp`). Each entry maps a name to a function returning the current value as a `String`:

```cpp
_variables.add("TEMP", [this]() { return String(_adc.getTemperature(), 1); });
_variables.add("UPTIME", []() { return String(millis() / 1000); });
```

Nothing else needs to change - the `SFVARHandler` serves every registered name. Rules:

- Use upper-case names without commas or `*`; `NACK` is reserved.
- The getter must be fast and non-blocking (it runs inside protocol processing).
- Return values without commas, so they do not break the field structure.
- Update the table above.

Implementation:

- `lib/SFProto/src/VariableRegistry.{h,cpp}` - name → getter map
- `lib/SFProto/src/structures/SFVAR.{h,cpp}` - request parsing
- `lib/SFProto/src/handlers/SFVARHandler.{h,cpp}` - responses
