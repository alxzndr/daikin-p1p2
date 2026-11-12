# Task 01 — CI build and capture tooling

**Branch:** `test/ci-and-capture` · **Depends on:** nothing · **Write impact:** none · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/TESTING.md`, `docs/ai/BUILD_AND_FLASH.md`,
`docs/ai/ARCHITECTURE.md`.

## Goal

Give the fork a safety net before any firmware change: every PR is built automatically, and the human
has one command to record real bus traffic for the offline tests of task 02.

## Deliverables

1. **`.github/workflows/build.yml`**: on every PR and on push to `main`, build
   - `examples/P1P2MQTT-bridge`: `Daikin-E-USB` and `Daikin-F-USB`
   - `examples/P1P2Monitor`: the Daikin E environment (check its `platformio.ini` for the exact name)

   Cache PlatformIO packages. Upload the E-series ESP `firmware.bin` and its SHA-256 as a build artifact.
   Print flash and RAM usage in the job summary. The workflow must not need any secret.
2. **`tools/capture.sh`**: wraps `mosquitto_sub` on `P1P2/R/#`, `P1P2/P/#`, `P1P2/S/#` and writes
   `captures/<date>_<time>_<label>.log` with a timestamp per line.
   - Broker host, user and password come from environment variables or a git-ignored `tools/capture.env`
     (commit a `tools/capture.env.example`). Never hard-code them.
   - Arguments: a label (`defrost`, `dhw_run`, `quiet_on`, `night_idle`, …) and an optional duration.
   - Subscribe only. The script must not contain any publish.
3. **`tools/scrub_capture.sh`** (or a flag): produces a copy that is safe to commit by dropping `P1P2/S`
   lines and anything matching credential-like patterns; it prints what it removed.
4. **`captures/README.md`**: the file format, the naming scheme, the list of situations worth capturing,
   and how to scrub. Add `.gitignore` rules so unscrubbed captures and `capture.env` cannot be committed.
5. Update `docs/ai/TESTING.md` section 1 to point at the script.

## Out of scope

- Running a capture against the real broker (the human does that; you have no credentials).
- Any change to firmware sources.

## Done when

- The workflow is green on your PR (link the run). If a job fails for a reason that already exists on
  `main`, report the exact error and stop; do not "fix" the platform or library pins.
- `shellcheck` passes on the scripts; you tested `capture.sh` against a local throwaway broker or a
  stubbed `mosquitto_sub` and say which.
- The human can follow `captures/README.md` without asking questions.
