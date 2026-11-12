# Task 04c — Write de-duplication (**write path — human-gated**)

**Branch:** `feat/write-dedup` · **Depends on:** tasks 02 and 04b merged ·
**Write impact:** yes, reduces writes · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/SAFETY.md` in full,
`docs/ai/WRITE_PATH.md` (from task 04b), `docs/ai/DAIKIN_UNIT.md`, `doc/Commands-Eseries.md`.

**This task changes which writes reach the heat pump. It has two phases. Do not start phase 2 until the
human has approved the phase 1 design in writing on the PR.**

## Goal

A write whose value equals the unit's confirmed current state is dropped on the ESP: it saves write
budget and possible EEPROM wear on the Daikin (see upstream issues #274, #277).

## Phase 1 — design note only (`docs/ai/design/write-dedup.md`, draft PR, no firmware change)

For **each** writable parameter the house uses (the entities in `docs/ai/HOUSE_CONTEXT.md`):

- the write command, and the bus field that proves the unit's current value, with packet evidence;
- whether "equal to the last *read back* value" is a safe reason to skip. It is **not** safe to compare
  with the last *commanded* value: the quiet-mode incident shows commanded and actual can differ;
- cases where a repeated identical write has a real effect and must never be dropped (DHW boost and
  other momentary/trigger actions, anything without a readback, values not yet seen after boot, values
  older than a freshness limit);
- the failure mode if the readback is stale or wrongly decoded, and how the user overrides it.

Proposed rules to challenge: allow-list per parameter, default **off**; skip only when a fresh readback
equals the request; a skipped write is always reported as an audit event (`skipped_duplicate`); a
runtime switch disables the feature without reflashing; no persistent state.

End phase 1 with the write-impact note in the `docs/ai/SAFETY.md` format and wait.

## Phase 2 — implementation (after approval)

- New self-contained module with a host-testable core; minimal hook in the command path.
- Host tests per allow-listed parameter: equal → skipped, different → passed through unchanged, no
  readback → passed through, stale readback → passed through, trigger action → always passed through.
- **Byte-for-byte test**: with the feature off, the serial output to the ATmega is identical to before.
- No retries, no re-assertion, no periodic writes. The feature can only ever send *fewer* writes.

## Out of scope

- ATmega changes, budget values, "verified quiet-mode off" (separate task).

## Done when

- Phase 1: the human can approve or reject per parameter.
- Phase 2: tests above pass in CI; build results reported; flash plan and a staged on-device test plan
  (one parameter, daytime, with the confirmation field to watch) are in the PR for the human to execute.
