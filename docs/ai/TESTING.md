# TESTING — without touching the heat pump

There is no simulator for a Daikin. Upstream has no automated tests. Building one is roadmap item #1,
because every later change depends on it.

## 1. Capture real traffic (read-only, safe)

The bridge publishes every raw packet on `P1P2/R/P1P2MQTT/bridge0`. Use the wrapper, it subscribes to
`P1P2/R/#`, `P1P2/P/#` and `P1P2/S/#`, timestamps every line and writes `captures/<date>_<time>_<label>.log`:

```bash
cp tools/capture.env.example tools/capture.env   # broker host/user/password (git-ignored, never commit)
tools/capture.sh -d 900 defrost                 # 15 minutes; without -d it runs until Ctrl-C
tools/scrub_capture.sh captures/<file>.log      # -> <file>_scrubbed.log, safe to commit
```

`tools/capture.sh` contains no publish and never touches `P1P2/W*` - reading the broker is safe, writing
is not (see `SAFETY.md`). Only the `*_scrubbed.log` files may be committed: `captures/.gitignore` blocks
the raw ones, and the scrubber drops every `P1P2/S` line plus anything credential-like and prints what it
removed. `captures/README.md` explains the line format, the naming scheme and the situations worth
capturing (`defrost`, `dhw_run`, `quiet_on`, `quiet_auto_stuck`, `buh_active`, `startup`, `night_idle`,
`boost_lwt38`).

`tools/capture.sh -D -d 30 contract` additionally captures the bridge's Home Assistant discovery payloads
(`homeassistant/<component>/bridge0/<object_id>/config`, narrow enough that other devices cannot leak in).
They are retained, so 30 seconds is enough. Task 03 diffs those payloads between two firmware versions;
they are kept by the scrubber and carry device identifiers, so read them before committing.

## 2. Offline decode harness (to build)

Goal: feed captured `R` lines into the ESP decoder on the Mac and compare the produced `P` parameters
with what the real bridge published at the same moment (golden file).

Approach that keeps the firmware untouched:
- Compile `P1P2_ParameterConversion.h` + the decode entry points with a host compiler (clang/g++),
  stubbing Arduino types (`String`, `millis()`, `Serial`) in `test/host/arduino_stub.h`.
- A small `test/host/replay.cpp` reads a capture, calls the decoder per packet, prints `KEY value`.
- `test/run_replay.sh <capture>` diffs against the `P` lines in the same capture.
- Run it in CI (GitHub Actions) on every PR together with `pio run -e Daikin-E-USB`.

Until that exists: any decoder change must include a hand-checked example (packet hex → expected value)
in the PR description.

## 3. Build tests

`.github/workflows/build.yml` builds all of the below on every PR and on `main`, prints flash/RAM in the
job summary and uploads the `Daikin-E-USB` `firmware.bin` with its SHA-256 as a build artifact.

- `pio run -e Daikin-E-USB` must pass; also `Daikin-F-USB` if you touched shared code (don't break other
  series users of the public fork).
- Watch flash/RAM usage in the build output; the ESP8266 is tight on RAM (LWIP low-memory variant).

## 4. On-device testing (human-gated)

- Read-only soak: flash, then compare 24 h of `P` values against the previous firmware's capture.
- Control tests one at a time, with an observable confirmation field, during the day, with the inverse
  command and its verification prepared (see SAFETY.md).
