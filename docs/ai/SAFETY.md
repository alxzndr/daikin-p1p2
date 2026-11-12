# SAFETY — what can go wrong, and the rules that prevent it

The bridge is connected in parallel to the room thermostat on the Daikin's P1/P2 bus. Reading is
harmless. **Writing changes the behaviour of a heat pump that heats a family home.**

## Failure modes (all realistic)

| Risk | How it happens | Consequence |
|---|---|---|
| Heating off / wrong mode | Wrong field id/value in a write, or a write the unit interprets differently on E-series | Cold house, frozen-pipe risk in winter |
| Backup heater (BUH, ~9 kW) forced on | Setpoint/LWT/booster writes, or a mode the unit answers with BUH | Huge consumption, capacity-tariff peak (expensive all month) |
| **Daikin EEPROM wear** | Some writes are stored in the unit's EEPROM (upstream #277: booster heater on/off). Repeated writes in a loop | Permanent damage to the indoor unit's controller |
| Stuck state | A write that is not undone by the "obvious" inverse. **Seen in this house (2026-11-07 → 11-12):** quiet mode set `On`, then set back to `Auto` → the unit stayed in quiet mode (Level 3) for 5 days, capping the compressor; only `Always off` cleared it | Silent loss of capacity for days |
| Bus collisions | `C1`/`C2`/`Q` and auxiliary-controller emulation deliberately bend P1/P2 timing (`doc/KnownIssues.md`); added traffic makes it worse | Thermostat/unit errors, Error_Budget dropping; bridge auto-disables functions |
| Write budget exhausted | Too many writes (HA automations, retries) — budget 100 per 15 min here | Commands silently refused (`writes_refused_budget` counter) — control loop thinks it acted |
| Bricked bridge | Wrong flash layout (`eagle.flash.4m.ld` is required — upstream warns 1m breaks OTA and is "dangerous for hardware"), interrupted OTA, ATmega flashed with wrong fuses/clock | No Daikin data/control until someone opens the box with an ESP01 programmer |
| Credential leak | Debug output of WiFi/MQTT credentials, telnet unauthenticated | LAN compromise of the heating controller |

## Hard rules

1. **No write path change without a written "write impact" note** in the PR: which bus packet/field,
   how often it can fire, whether the unit stores it in EEPROM (assume yes if unknown), what the
   inverse is and how to verify the inverse worked (read back a status, not just the setpoint).
2. **Never add periodic or retry writes** to the P1/P2 bus. Re-assert only on an observed mismatch,
   rate-limited, and through the existing write budget.
3. **Writes must go through the write budget.** Do not bypass or raise it in code. Budget defaults
   live in `P1P2_Config.h`; the house runs 100 per 15 min.
4. **Every control must have an observable confirmation.** If you add/modify a control, publish (or
   document) the status field that proves the unit accepted it (e.g. `Quiet_Mode_Active`, not only
   `Quiet_Mode` setpoint).
5. **No flashing by agents.** Prepare: the binary path + SHA-256, the PlatformIO env, the exact
   upload command, and the rollback command (`docs/ai/BUILD_AND_FLASH.md`). The human flashes.
6. **ESP first, ATmega last.** Most improvements belong in the ESP decoder/HA layer. Touch
   `P1P2MQTT.cpp` (bus timing/interrupts) or `P1P2Monitor` only with a clear reason and logic-analyser-
   or log-level evidence; treat timing changes as high risk.
7. **Seasonal caution.** From October to April the heat pump is load-bearing. Flash only at a moment the
   human picks (daytime, mild weather, someone home), never late evening, never right before a cold night.
8. **No secrets in the repo.** WiFi/MQTT/OTA credentials are configured on the device (WiFiManager /
   EEPROM). Never commit them, never print them in logs you add.

## Pre-flash checklist (the human signs off)

- [ ] Builds clean for the exact env; binary size within the 4M layout; SHA-256 recorded.
- [ ] Offline decode tests pass against captured raw logs (`docs/ai/TESTING.md`).
- [ ] Diff reviewed for any write-path change; write-impact note present if so.
- [ ] Rollback image available locally: `Firmware_images/P1P2MQTT-bridge-v0.9.58rc23-Daikin-E.ino.bin`.
- [ ] HA side ready: automations that drive the Daikin can be paused (`input_boolean.emhass_enabled`).
- [ ] Post-flash checks listed: bridge back on MQTT, `bridge0_*` entities available, Error_Budget stable,
      write budget sane, a read-only value (LWT/RWT) updating, `Quiet_Mode_Active` = 0.
