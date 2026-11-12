# test/host — the ESP firmware, compiled for your laptop

This directory builds the **unmodified ESP8266 firmware sources** with the host C++
compiler so the decoder can be run against captured bus packets. No flashing, no broker,
no heat pump. It is a test tool: it is not a firmware build and it does not change the
PlatformIO build (`.github/workflows/build.yml` still compiles the real thing).

```
stubs/                 Arduino + ESP8266 API replacements (String, Serial, WiFi, MQTT, EEPROM, …)
stubs/host_env.h       what gets force-included before the sketch, in the right order
gen_prototypes.py      generates the function declarations the Arduino build inserts for you
replay.cpp             reads a capture, feeds the ATmega lines to the firmware, records publishes
Makefile               the host build
compare.py             readable diff against a golden file or against the capture's own P lines
build/<SERIES>/replay  the build artifact (git-ignored)
```

## Use it through test/run_replay.sh

```bash
test/run_replay.sh test/fixtures/daikin_e_doc_packets.log              # expectations + golden
test/run_replay.sh captures/<file>.log --against-capture               # vs what the bridge published
test/run_replay.sh captures/<file>.log --update-golden                 # accept a new reference
test/run_replay.sh <capture> --series F_SERIES                         # another decode table
make -C test/host FIRMWARE_DIR=/tmp/rc23/examples/P1P2MQTT-bridge BUILD_DIR=/tmp/rc23-host
test/run_replay.sh <capture> --replay-bin /tmp/rc23-host/replay --update-golden   # rc23 reference
```

Nothing leaves the process: publishes are appended to a list and printed, they are never
sent anywhere. `AsyncMqttClient` in `stubs/` has no sockets.

## How it gets the firmware to compile

The `.ino` is included as one translation unit through `stubs/host_env.h`, which first
pulls in the standard headers (the firmware's `min`/`max`/`abs` macros in `Arduino.h`
would otherwise break `<string>`), then the firmware's leaf headers, then the generated
prototypes, then the sketch itself. Consequences worth knowing:

- **`g++`, not Apple clang.** The firmware casts an address to `uint32_t` (ESP8266 is
  32 bit); clang treats pointer-to-int casts as an error on a 64-bit host, GCC accepts
  them with `-fpermissive`. The Makefile picks the first GCC it finds (`g++-16`, `g++-15`,
  …, `g++`) and adds the macOS SDK path for Homebrew GCC. On Linux the plain `g++` works.
- **One firmware line changed** for the host build: `PARAM_NR` used
  `sizeof(paramName) / 4`, which is only right while a pointer is 4 bytes. It is now
  `sizeof(paramName) / sizeof(paramName[0])` — same value on the ESP8266. `firmware.bin`
  for `Daikin-E-USB` is byte-identical to the one built from `origin/main` apart from the
  `__DATE__`/`__TIME__` strings and the 5 bytes that already differ between two builds of
  identical source; size 848048 in both. See the PR that added this harness.
- No `#ifdef HOST_REPLAY_BUILD` was added to the firmware; the stub headers do all the work.

## What the stubs do

| Stub | Behaviour |
|---|---|
| `AsyncMqttClient` | `connect()` succeeds, `publish()` appends to the recorded list. LWT topics are recorded too, so the birth/availability contract is visible. |
| `Serial` (ESP side) | the ATmega link: `Serial.available()`/`read()` drain lines that `replay.cpp` pushes in; `write()`/`print()` to the ATmega are swallowed. |
| `EEPROM` | starts "unread", so the firmware falls back to its compiled defaults, exactly like a factory-reset bridge. `--output-mode <hex>` and `--output-filter <n>` override the two output switches
  (`EE.outputMode` defaults to `0x0002` = parameters only for E-series, `EE.outputFilter` to 1). |
| `WiFi`, `WiFiServer`, `ESP8266WebServer`, `ESPTelnet`, `DNSServer`, `mDNS`, `ArduinoOTA`, `Update`, `FS` | no-ops. WiFi is reported connected with a fixed RSSI of -50 dBm. |
| `millis()`/`delay()`/`micros()` | a virtual clock. `delay()` advances it, and feeding a capture line advances it to that line's timestamp, so `millis()`-based bookkeeping inside the firmware behaves as it did in the house. |
| `localtime_r`/`time()` | wall clock derived from the capture's own timestamps, with the firmware's POSIX TZ (`configTZ` is recorded, the harness reads it from the firmware's own `configTZ` string). |
| `ESP` | `getFlashChipSize()` etc. return sane constants; `restart()` aborts the replay with a message instead of looping. |

## What this harness does *not* model

Say this out loud when you use it as evidence:

- **The ATmega side is not covered at all.** `P1P2MQTT.cpp`, the interrupt-driven bus
  timing and `examples/P1P2Monitor/` are not compiled here. This proves how the ESP
  *decodes* bytes, never how bytes are acquired off the bus.
- The ESP starts from an empty state and from compiled EEPROM defaults, so it publishes
  every key once when first seen, and it does not know the house's configuration
  (throttling, offsets, entity enable flags, presets are defaults).
- Virtual uptime at the first fed line is whatever `setup()`'s `delay()` calls add up to
  (about a minute of virtual time), not the real bridge's uptime. Anything keyed on
  uptime (`ESP_Uptime`, `ATmega_Uptime`, counter scheduling) will not match a capture.
- The startup throttle ramp is reset after `setup()` (`throttleValue = 0`, `throttle = 1`) because a
  capture normally starts long after boot. Pass `--no-throttle-skip` to keep the firmware's own
  ramp (`THROTTLE_VALUE` counting down); the first ~100 virtual seconds then drop most packets,
  exactly as a bridge that just restarted does.
- Only what the firmware itself decides to publish is produced: the harness never invents
  a message, and it never fills gaps that the real bridge published before the capture
  started (those arrive as `retained=1` lines and are skipped as initial state for `R`).
