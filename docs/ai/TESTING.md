# TESTING — without touching the heat pump

There is no simulator for a Daikin. Upstream has no automated tests. Building one is roadmap item #1,
because every later change depends on it.

## 1. Capture real traffic (read-only, safe)

The bridge publishes every raw packet on `P1P2/R/P1P2MQTT/bridge0`. Capture with a subscriber on the
broker (HA host 10.0.1.5, port 1883; credentials from the human — never commit them):

```bash
mosquitto_sub -h 10.0.1.5 -u <user> -P <pass> -v \
  -t 'P1P2/R/#' -t 'P1P2/P/#' -t 'P1P2/S/#' | ts '%Y-%m-%dT%H:%M:%S' > captures/$(date +%F_%H%M).log
```

Capture interesting situations deliberately and name them: `defrost`, `dhw_run`, `quiet_on`,
`quiet_auto_stuck`, `buh_active`, `startup`, `night_idle`, `boost_lwt38`. Keep them in `captures/`
(raw bus data contains no secrets; check before committing anything with `P1P2/S` lines).

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

- `pio run -e Daikin-E-USB` must pass; also `Daikin-F-USB` if you touched shared code (don't break other
  series users of the public fork).
- Watch flash/RAM usage in the build output; the ESP8266 is tight on RAM (LWIP low-memory variant).

## 4. On-device testing (human-gated)

- Read-only soak: flash, then compare 24 h of `P` values against the previous firmware's capture.
- Control tests one at a time, with an observable confirmation field, during the day, with the inverse
  command and its verification prepared (see SAFETY.md).
