# AGENTS.md — working on daikin-p1p2

This repository is a fork of [Arnold-n/P1P2MQTT](https://github.com/Arnold-n/P1P2MQTT): firmware for a
bridge that sits on the P1/P2 thermostat bus of a **live Daikin Altherma heat pump** and exposes it to
Home Assistant over MQTT. The goal of the fork is a better, safer, better-tested version for one real
house (see `docs/ai/HOUSE_CONTEXT.md`), developed together with AI agents.

**Read this file first, then the doc in `docs/ai/` that matches your task.**

## The three rules that override everything else

1. **This code talks to a real heating system.** A wrong write can switch heating off in winter, force
   the 9 kW backup heater on, wear out the Daikin's EEPROM, or brick the bridge. Never flash, never send
   bus or MQTT write commands, and never change a write path without the human's explicit approval
   for that specific action. Read `docs/ai/SAFETY.md` before touching anything under "write".
2. **The license is CC BY-NC-ND 4.0 with exceptions** (`LICENSE.md`). Modified code may only be
   distributed as a public GitHub fork under the same license, with attribution, non-commercially.
   Keep every existing copyright/license header. Never copy this code into another project or a
   private package. See `docs/ai/CONVENTIONS.md`.
3. **Home Assistant depends on the current MQTT/entity contract.** Automations in the house drive
   `climate.bridge0_*`, `select.bridge0_quiet_*`, `sensor.bridge0_*`. Do not rename topics, keys,
   unique_ids or units unless the task is explicitly about that, and then list every affected entity.

## Repo map

| Path | What it is |
|---|---|
| `P1P2MQTT.cpp`, `P1P2MQTT.h` | The bus library (timing-critical, interrupt-driven, AVR). Based on AltSoftSerial (MIT notice inside). |
| `examples/P1P2Monitor/` | **ATmega328P** firmware: reads/writes the P1/P2 bus, emulates an auxiliary controller, serial link to the ESP. PlatformIO, MiniCore 8 MHz. |
| `examples/P1P2MQTT-bridge/` | **ESP8266** firmware (~4.3k-line `.ino` + headers): WiFi, MQTT, HA discovery, telnet, web OTA, decoding. Build flag `-D E_SERIES` for our unit. |
| `examples/P1P2MQTT-bridge/P1P2_ParameterConversion.h` | ~6.4k lines: packet → parameter decoding per model. Where most bugs and most improvements live. |
| `examples/P1P2MQTT-bridge/P1P2_HomeAssistant.h` | MQTT discovery (entity names, device classes, units). |
| `examples/P1P2MQTT-bridge/P1P2_Config.h` | Versions, compile-time switches, defaults (write budget etc.). |
| `OSS-dependencies/` | Pinned, partly *modified* ESP8266 core 3.0.2 + libraries. Builds depend on these exact zips. |
| `Firmware_images/` | Upstream prebuilt binaries. `P1P2MQTT-bridge-v0.9.58rc23-Daikin-E.ino.bin` = what runs in the house today = the **rollback image**. |
| `doc/` | Upstream docs: protocol (`LogicalFormat/`, `PhysicalFormat/`), commands, HA guide, known issues, security. |
| `docs/ai/` | **Fork docs for agents** (this fork only). |

## Where to read next

| Task | Read |
|---|---|
| Anything that writes to the bus, changes control entities, budgets, EEPROM | `docs/ai/SAFETY.md` |
| Understanding the two-MCU design, data flow, MQTT topics | `docs/ai/ARCHITECTURE.md` |
| Building, flashing, rolling back | `docs/ai/BUILD_AND_FLASH.md` |
| Testing without touching the heat pump | `docs/ai/TESTING.md` |
| Our unit, our HA entities, known field problems | `docs/ai/HOUSE_CONTEXT.md` |
| The Daikin itself: models, capacities, counters, settings, field settings, quirks | `docs/ai/DAIKIN_UNIT.md` |
| Code style, commits, branches, versioning, license | `docs/ai/CONVENTIONS.md` |
| What we want to improve and in what order | `docs/ai/ROADMAP.md` |

## Working agreement for agents

- **Small, reviewable changes.** One concern per branch/PR. No drive-by reformatting of the big files —
  diffs in a 6k-line header must stay readable.
- **Build before you claim.** Every code change must compile for `Daikin-E-USB`
  (`pio run -e Daikin-E-USB` in `examples/P1P2MQTT-bridge/`), and for the ATmega if you touched the
  library or `P1P2Monitor`. Report the exact command and result. "Should compile" is not acceptable.
- **Prove decoding changes offline** with captured raw packets (`docs/ai/TESTING.md`) before anyone flashes.
- **Never flash or send commands yourself.** Prepare the binary, the exact flash command and the
  rollback command; the human runs or approves them.
- **Respect the bus.** Do not add polling, retries or periodic writes on the P1/P2 bus without a written
  rationale on timing, write budget and EEPROM impact.
- **Write down what you learn.** Protocol findings go in `docs/ai/` (or upstream `doc/` style) with the
  raw packet evidence; field incidents go in `docs/ai/HOUSE_CONTEXT.md`.
- **Upstream first where possible.** Generic fixes (not house-specific) should be prepared so they can be
  offered back to Arnold-n/P1P2MQTT as a PR from this public fork.
- Answer the human in the language they use (usually Dutch); code, comments and commit messages in English.
