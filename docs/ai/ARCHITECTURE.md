# ARCHITECTURE — how the bridge works

```
Daikin indoor unit ── P1/P2 bus (HBS, 2-wire, bus-powered) ──┬── room thermostat (Madoka/…)
                                                             └── P1P2MQTT bridge v1.2
                                                                  MAX22088 HBS adapter
                                                                  ATmega328P  (P1P2Monitor)
                                                                     │ serial
                                                                  ESP8266     (P1P2MQTT-bridge)
                                                                     │ WiFi
                                                     MQTT broker (Mosquitto on the HA host)
                                                                     │
                                                     Home Assistant (MQTT discovery → bridge0_* entities)
```

## Two microcontrollers, two firmwares

| | ATmega328P — `examples/P1P2Monitor/` | ESP8266 — `examples/P1P2MQTT-bridge/` |
|---|---|---|
| Job | Bit-level bus I/O with exact timing (uses `P1P2MQTT.cpp`), packet framing, error detection, acting as **auxiliary controller** (answers 0x00F03x requests so it can inject parameter writes), counter requests (`C2`) | Receives raw packets over serial, decodes them to parameters, publishes MQTT, HA discovery, receives HA commands, enforces write budget, telnet, WiFiManager, OTA (ESP and ATmega) |
| Build | PlatformIO, MiniCore, 8 MHz external, BOD 2.7 V, `eesave=yes` | PlatformIO, espressif8266 with **modified** core 3.0.2 from `OSS-dependencies/`, `esp12e`, 4M layout |
| Series flag | `-D E_SERIES` (Daikin Altherma E-series = ours) | `-D E_SERIES` |
| Versions in the house | P1P2Monitor (see `ATmega` version in HA/telnet) | **v0.9.58rc23** (2025-06-07); upstream source is at rc34+ |

Most decoding and all HA logic is on the ESP. The ATmega is the risky part (timing, bus behaviour).

## Data flow

1. ATmega reads packets → sends hex lines to the ESP (`R` lines); also emits pseudo-packets with its own
   status.
2. ESP `P1P2_ParameterConversion.h` maps `(packet type, source, byte offset)` → named parameter with
   conversion (temperature f8.8, flags, counters…). Model-specific tables are selected by `*_SERIES`.
3. Changed values are published on `P1P2/P/P1P2MQTT/bridge0/<X>/<SRC>/<KEY>`; HA discovery configs are
   published once per entity (`P1P2_HomeAssistant.h`).
4. HA commands arrive on the bridge's command topics (`P1P2/W`, HA climate/select/number command topics)
   → ESP validates, checks write budget → asks the ATmega to inject a write in the next auxiliary-
   controller response window.

## MQTT topics (upstream `doc/P1P2MQTT.md`)

| Topic | Content |
|---|---|
| `P1P2/P/P1P2MQTT/bridge0/<X>/<SRC>/<KEY>` | One parameter per topic. `X` = category letter (T temperature, C counter, A bridge…) |
| `P1P2/R/P1P2MQTT/bridge0` | Raw hex packets (+ pseudo-packets) — **the input for offline tests**. Only published when output mode bits `0x0001` (bus packets) and `0x0004` (pseudo-packets) are set; the house bridge runs `0x0002`, so this topic is silent today (`TESTING.md` §1) |
| `P1P2/S/P1P2MQTT/bridge0` | Status, errors, verbose info |
| `P1P2/W[/...]` | Commands to the bridge (dangerous — see SAFETY.md) |
| `P1P2/L/P1P2MQTT/bridge0` | Availability (`online`/`offline`) |
| `homeassistant/<component>/bridge0/…/config` | HA discovery |

## Things that are easy to get wrong

- **Counters**: whole kWh, updated ~hourly, only with `Counter_Request_Function` on.
  `Electricity_Consumed_Compressor_Heating` is frozen at 16777 kWh on some units (≈2^24 Wh) — ours too.
- **Setpoint vs. state**: HA shows the commanded setpoint immediately; the unit's acceptance must be read
  back from a status field.
- **Quiet mode** has a commanded mode (`Auto`/`Always off`/`On`), a level, and a separate *active* flag.
  `Auto` does not necessarily clear a previously commanded `On` (house incident 2026-11).
- **Discovery is cached by HA**: changing a discovery payload may require `MQTT_Rebuild` and can leave
  orphaned entities; unique_ids must stay stable.
