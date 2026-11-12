# Task 04a — Commanded-versus-active mismatch detection (read path)

**Branch:** `feat/mismatch-detection` · **Depends on:** task 02 merged (task 03 preferred) ·
**Write impact:** none allowed · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/SAFETY.md`, `docs/ai/HOUSE_CONTEXT.md`
(incident 2026-11-07 → 11-12), `docs/ai/DAIKIN_UNIT.md`, `docs/ai/ARCHITECTURE.md`.

## Goal

For five days the unit ran in quiet mode Level 3 while the commanded mode was `Auto`; nobody noticed.
The bridge must report when what was commanded and what the unit actually does disagree.

## Design constraints

- **New logic lives in a new, self-contained module** (new header/source under
  `examples/P1P2MQTT-bridge/`), written as plain C++ with no Arduino dependencies in its core, so it is
  unit-tested on the host in `test/host/`. The existing files only get a small hook that feeds it
  decoded values and publishes its result.
- **Read-only.** The module never sends or schedules a write, never re-asserts anything.
- **Additive contract.** Only new entities and topics; nothing existing is renamed or changed.
- Fixed memory, no heap allocation, no `String`. The ESP8266 is tight on RAM.

## Deliverables

1. **Investigation first** (in the PR and in `docs/ai/`): which bus fields carry the commanded quiet
   mode, the quiet level and the *active* flag, with packet type, source, offset and raw examples from
   the captures. Describe what `Auto` means on this unit and when "active while Auto" is legitimate
   (a quiet schedule on the MMI) versus stuck. If the captures cannot answer this, say what capture is
   missing; do not guess.
2. **The detector**: generic "pair" mechanism (commanded value, observed value, rule, grace period),
   with quiet mode as the first and only configured pair. Proposed behaviour: mismatch is raised when
   the commanded mode says off and the active flag stays on longer than a grace period (default to be
   justified from the observed readback lag, see "Write latency" in `DAIKIN_UNIT.md`); cleared as soon
   as they agree.
3. **Publishing**: one binary sensor per pair via HA discovery (`problem` device class) plus a line on
   the status topic when it raises and clears. List the exact new topic, key, unique_id and discovery
   payload in the PR.
4. **Tests**: host unit tests for the state machine (raise, clear, grace period, restart, value
   unknown at boot), and a replay test. If a capture of the stuck situation does not exist, build a
   synthetic one from real packets and label it synthetic.
5. Build results with flash/RAM delta.

## Out of scope

- Automatic correction ("re-assert Always off"). That is a write and a separate, human-gated task.
- Other pairs (setpoint accepted, DHW boost…): propose them in the PR, do not implement.

## Done when

- Replaying the stuck scenario raises the sensor; replaying normal captures never does.
- All existing replay goldens are unchanged except for the added keys.
- The PR states whether this is an upstream candidate and which commits are generic.
