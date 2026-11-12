# ROADMAP — what "improved" means for this fork

Ordered by value ÷ risk. Each item becomes its own branch/PR. Agents: pick from the top, confirm with
the human before starting anything marked **write**.

## Phase 0 — foundation (no device changes)

1. **Fork + CI**: public GitHub fork, GitHub Actions building `Daikin-E-USB`, `Daikin-F-USB`,
   `P1P2Monitor Daikin-E` on every PR.
2. **Capture tooling**: `tools/capture.sh` (mosquitto_sub → timestamped log), a first set of named
   captures from the house (see TESTING.md).
3. **Offline replay harness** (`test/host/`): decode captures on the Mac, golden-file diff. Gate every
   decoder change on it.
4. **Record the running versions** (ESP rc23, ATmega version) and keep their binaries as rollback.

## Phase 1 — catch up with upstream safely (read-path only)

5. Build current upstream (rc34+) for E-series, replay captures, compare with rc23 output; list
   differences (renamed keys, discovery changes) before proposing a flash.
6. Fix/verify the `Defrost_Active_param15` discovery `device_class` (HA error at every start).

## Phase 2 — observability (mostly read-path)

7. **Commanded-vs-active mismatch detection** for quiet mode (and similar mode/level pairs): publish a
   `*_Mismatch` binary sensor and a status line when the commanded mode and the unit's active flag
   disagree for > N minutes. (House incident 2026-11-07 → 11-12.)
8. **Write audit log**: publish every accepted/refused write with reason and remaining budget on a
   status topic, so HA can see refused writes (today only a saturating counter).
9. **Better counters**: use the total counter when the heating counter is frozen (`useTotal`), expose
   which source is used.
10. **Thermal power / COP from bus values** with documented formula (flow × ΔT), so the bridge reports
    something consistent with HA's own calculation.

## Phase 3 — control quality (**write**, human-gated)

11. **Write de-duplication**: never send a write whose value equals the last confirmed state.
12. **Verified quiet-mode off**: `Always off` as the documented "off", and a re-assert only on
    confirmed mismatch, budgeted.
13. Investigate upstream #277 (booster heater writes EEPROM) and #274 (HC on/off vs write budget) on our
    unit with captures before changing anything.

## Phase 4 — security

14. Telnet lock and OTA password documented and set on the device (configuration, not code).
15. Optional: protect the web OTA page (upstream Security.md lists it as unprotected).

## Explicitly out of scope (for now)

- Changing bus timing in `P1P2MQTT.cpp` or the auxiliary-controller emulation.
- Supporting other brands/series beyond "keep them compiling".
