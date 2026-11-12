# Task 02 — Offline replay harness (host-side decoder tests)

**Branch:** `test/replay-harness` · **Depends on:** nothing (uses task 01's capture format; see below) ·
**Write impact:** none · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/TESTING.md`, `docs/ai/ARCHITECTURE.md`,
`doc/P1P2Monitor-commands.md` (raw line format), `doc/P1P2MQTT.md` (topics).

## Goal

Run the **unmodified** ESP decoder on the Mac: raw `R` lines in, the MQTT messages the bridge would
publish out. This is the gate for every later decoder change and for the rc23 → rc34 upgrade (task 03).

## Deliverables

1. **`test/host/`**: a host build (clang++/g++, plain Makefile or CMake) that compiles
   `examples/P1P2MQTT-bridge/P1P2_ParameterConversion.h` and the decode entry points it needs, with
   `-D E_SERIES`, against stubs for the Arduino/ESP environment (`String`, `millis()`, `Serial`, PROGMEM
   macros, EEPROM/RTC storage, the MQTT publish function).
   - The publish stub records `topic payload` lines instead of sending.
   - **Do not edit the firmware sources to make this work.** If a tiny, behaviour-neutral change is
     truly unavoidable (for example an `#ifndef` guard), keep it in its own commit, explain it, and
     prove the ESP build output is unchanged in size and behaviour.
   - Time is driven by the capture's timestamps, not the wall clock, so runs are deterministic.
2. **`test/host/replay`**: reads a capture (format of `tools/capture.sh`: timestamp, topic, payload;
   if task 01 is not merged yet, support the raw `mosquitto_sub -v` format described in
   `docs/ai/TESTING.md` and say so), feeds the `P1P2/R` lines to the decoder, prints the produced
   parameter topics/values and, separately, the HA discovery payloads.
3. **`test/run_replay.sh <capture> [--update-golden]`**: compares the output with a golden file and
   exits non-zero on any difference, with a readable diff (key, old value, new value).
4. **A second comparison mode**: compare the replay output with the `P1P2/P` lines **in the same
   capture** (what the real bridge published). Report matches, mismatches and keys only present on
   one side. Expect and document legitimate differences (publish-on-change, timing, pseudo-packets).
5. **Seed test data**: until real captures exist, build a small fixture from the packet examples in
   `doc/LogicalFormat/` and `doc/P1P2Monitor-commands.md`, with a few hand-checked expectations
   (packet hex → key → value, shown in the PR).
6. **CI**: add a job that builds the harness and runs the fixtures (extend task 01's workflow if it is
   merged, otherwise add `.github/workflows/replay.yml`).
7. **Decoder version switch**: the harness must be able to build against another checkout of the
   decoder (a path argument), so task 03 can replay the same capture through rc23
   (commit `07e7230`) and through current `main`.
8. Update `docs/ai/TESTING.md` section 2 from "to build" to how it actually works.

## Out of scope

- Fixing decoder bugs you find. List them in the PR with the packet evidence; they become separate tasks.
- The write/command path (HA command → ATmega). Stub it; do not exercise it.
- F-series and other brands (keep the design open for them, do not implement).

## Done when

- `test/run_replay.sh` passes on the fixtures locally and in CI.
- `pio run -e Daikin-E-USB` output is byte-identical or you explain every difference.
- The PR lists exactly which firmware functions are stubbed and what the harness therefore cannot detect.
