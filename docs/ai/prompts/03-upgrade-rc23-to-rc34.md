# Task 03 — Catch up with upstream: rc23 → current, proven offline

**Branch:** `feat/upstream-rc34` · **Depends on:** task 02 merged, and real captures from the house in
`captures/` (the human records them with task 01's script) · **Write impact:** must be assessed ·
**Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/SAFETY.md`, `docs/ai/HOUSE_CONTEXT.md`,
`docs/ai/DAIKIN_UNIT.md`, `docs/ai/BUILD_AND_FLASH.md`, `doc/ReleaseNotes.md`.

## Goal

The house runs ESP firmware **0.9.58rc23** (source: commit `07e7230`). Our `main` is at upstream rc34+.
Produce the evidence the human needs to decide on flashing the current version, and a flash-ready
binary. **You do not flash.**

## Deliverables

1. **`docs/ai/UPGRADE_rc23_to_rc34.md`** containing:
   - **Source diff review** `07e7230..main` limited to `examples/P1P2MQTT-bridge/`: every change grouped
     as decoding / discovery / write path / networking / other, with file:line.
   - **Replay diff**: each house capture replayed through the rc23 decoder and the current decoder.
     Table of every key whose value, topic, name, unit, device_class or unique_id differs, added or
     disappeared.
   - **HA contract impact**: for every entity in the table of `docs/ai/HOUSE_CONTEXT.md` ("How Home
     Assistant uses the bridge"), state unchanged / changed (how) / not covered by any capture.
     You may read current entity ids and states through the HA MCP tools, **read-only**.
   - **Write-path changes**: each change to command handling, write budget interaction or what is sent
     to the ATmega, with a write-impact note in the `docs/ai/SAFETY.md` format. Unknown EEPROM
     behaviour is recorded as "assume yes".
   - **ATmega compatibility**: which P1P2Monitor version current ESP firmware expects versus what is in
     the house (the house version is "to be recorded" — say what the human must read out and where).
     The intent is to flash the ESP only. If that is not safe, say so plainly.
   - **Verdict**: flash / flash after X / do not flash, with reasons.
2. **Version suffix** per `docs/ai/CONVENTIONS.md` (`…-ag.1`) and a first `docs/ai/CHANGELOG.md` entry.
3. **Flash plan** in the PR: binary path, SHA-256, size versus the 4M layout, exact upload command,
   rollback command, post-flash checks (from `docs/ai/BUILD_AND_FLASH.md`), and the HA automations to
   pause first.
4. **Verify the `Defrost_Active_param15` discovery fix** (HA rejects `device_class: temperature` today):
   show the old and new discovery payload from the replay.

## Out of scope

- Any code change beyond the version suffix. Problems you find are listed, not fixed.
- Changes to ATmega firmware.
- Flashing, or asking HA to do anything.

## Done when

- The document answers "what will change in my Home Assistant after this flash?" entity by entity.
- Gaps are explicit: situations with no capture (defrost, BUH, DHW run, quiet mode…) are listed as
  untested, with the capture the human should still record.
