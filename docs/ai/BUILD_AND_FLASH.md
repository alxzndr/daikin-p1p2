# BUILD AND FLASH

## Build (agents may do this)

```bash
# ESP8266 bridge, Daikin E-series (our unit)
cd examples/P1P2MQTT-bridge
pio run -e Daikin-E-USB            # output: .pio/build/Daikin-E-USB/firmware.bin
shasum -a 256 .pio/build/Daikin-E-USB/firmware.bin

# ATmega328P monitor (only if the library or P1P2Monitor changed)
cd ../P1P2Monitor
pio run -e Daikin-E                # output: .pio/build/Daikin-E/firmware.hex
```

Notes
- `platformio.ini` uses the **modified** ESP8266 core and libraries from `../../OSS-dependencies/`.
  Do not switch to the registry versions to "fix" a build; that changes networking behaviour.
- `extra_scripts = flash_external.py` and `external_binary_path` let PlatformIO upload a prebuilt binary
  instead of the fresh build — check which one an upload command would actually push.
- `board_build.ldscript = eagle.flash.4m.ld` is mandatory (1m layout breaks OTA and can damage hardware).
- Bump the version suffix for every build that might be flashed (see `CONVENTIONS.md`), so HA's
  `sw_version` (`0.9.58rc23` today) tells you what is running.
- `pio` is on the Mac via Homebrew (`/opt/homebrew/bin/pio`). First build downloads toolchains (~minutes).

## Flash (the human does this — agents prepare the commands)

The bridge in the house: **192.168.2.110** (IoT VLAN).

| Path | How | Notes |
|---|---|---|
| ESP OTA (PlatformIO) | `pio run -e Daikin-E-OTA -t upload --upload-port 192.168.2.110` | espota with `--auth=<OTA password>` (configured on the device with `P18`; never commit the real one). Requires the Mac to reach UDP on the IoT VLAN. |
| ESP web upload | Browser → `http://192.168.2.110/` → "Firmware ESP8266" | Unauthenticated page; simplest path. |
| ATmega | Via the bridge (ESP flashes the ATmega; see upstream `doc/` and the web page "Firmware ATmega" when present) | Highest risk. Only with the human present. |
| Recovery | ESP01-compatible programmer on the board connector, `Daikin-E-USB` env with esptool | Requires opening the enclosure. |

### Rollback

- ESP: flash `Firmware_images/P1P2MQTT-bridge-v0.9.58rc23-Daikin-E.ino.bin` (what ran before the fork).
- ATmega: `Firmware_images/P1P2Monitor-v0.9.59rc2-Daikin-E.ino.hex` (verify it matches what ran before;
  record the ATmega version from HA/telnet *before* the first flash).
- EEPROM settings (WiFi/MQTT/offsets) normally survive an ESP update; if MQTT does not reconnect, use the
  bridge AP (see upstream `doc/HomeAssistant.md`).

### Post-flash verification (copy into the PR)

1. `P1P2/L/P1P2MQTT/bridge0` = `online`; HA `bridge0_*` entities not `unavailable`.
2. HA device `sw_version` shows the new version.
3. Live values move: `sensor.bridge0_sensors_temperature_r2t_leaving_water`, `..._r4t_return_water`, flow.
4. `sensor.bridge0_bridge_error_budget` stable (≥ 9, not decreasing), `writes_refused_*` not climbing.
5. `sensor.bridge0_quiet_quiet_mode_active` = 0 (unless intentionally on).
6. One harmless control round-trip only if the human asks (e.g. DHW setpoint unchanged re-write) and
   read back the status.
