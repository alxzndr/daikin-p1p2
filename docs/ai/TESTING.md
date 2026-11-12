# TESTING — without touching the heat pump

There is no simulator for a Daikin and upstream has no automated tests. Section 2 is the harness we
built for that (roadmap item #1): the real ESP decode code, compiled for the laptop, replaying captured
bus packets. Section 1 produces the input it needs.

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

## 2. Offline decode harness (`test/host/`, runs on the Mac and in CI)

The ESP firmware sources are compiled for the host with the Arduino/ESP8266 API stubbed out
(`test/host/stubs/`), a capture's `P1P2/R` lines are fed back in as the ATmega's serial lines, and
the messages the firmware *would* publish are printed instead of sent. Nothing is flashed, no broker
is contacted, `P1P2MQTT.cpp` and `examples/P1P2Monitor/` are not involved.

```bash
test/run_replay.sh <capture>                       # hand-checked expectations + golden file
test/run_replay.sh <capture> --against-capture     # vs the P1P2/P lines of the same capture
test/run_replay.sh <capture> --update-golden       # accept a new reference after review
make -C test/host                                  # build only (E-series; SERIES=F_SERIES for other)
```

Output columns are `<virtual seconds> <channel> <retain> <topic> <payload>` with channel
`P` parameter, `D` Home Assistant discovery, `S` bridge status, `R` raw hex, `O` other; `P` and `D` are
on by default, `S`/`R` need `-- --include-status --include-raw` (everything after `--` goes to the binary).

Three kinds of evidence, all machine checked:

1. **Golden file** (`test/golden/<fixture>.<SERIES>.golden.txt`): the complete message sequence this
   decoder produces. A changed byte offset in `P1P2_ParameterConversion.h` shows up as a changed value,
   a renamed key or a changed discovery payload. Never accept a `--update-golden` you cannot explain
   key by key in the PR.
2. **Hand-checked expectations** (`test/fixtures/<fixture>.<SERIES>.expect`): `packet hex → key → value`
   derived from the protocol tables in `doc/LogicalFormat/`, *not* copied from the harness output. See
   `test/fixtures/daikin_e_doc_packets.E_SERIES.expect` for the format (`count:N`, `contains:`).
3. **Live-vs-replay** (`--against-capture`): compares with what the bridge in the house published in the
   same window. Expect differences by construction - the replay starts from an empty state and from
   compiled EEPROM defaults, so it publishes each key once on first sight, and pseudo/uptime keys never
   line up. The number to look at is `final value differs`: that should be 0 for the keys both sides
   have.

Comparing two firmware revisions over the same packets (this is how rc23 and `main` get compared in
roadmap task 03):

```bash
make -C test/host FIRMWARE_DIR=/tmp/rc23/examples/P1P2MQTT-bridge BUILD_DIR=/tmp/rc23-host
test/run_replay.sh <capture> --replay-bin /tmp/rc23-host/replay --golden /tmp/rc23.golden.txt
test/run_replay.sh <capture> --golden /tmp/rc23.golden.txt        # now the diff is main vs rc23
```

CI: `.github/workflows/replay.yml` builds the harness, runs the fixtures for `E_SERIES` and
`F_SERIES`, checks that the run is reproducible and that building the firmware from a second checkout
gives the same answer. It needs no secrets and no hardware.

Limits to state when you quote this as evidence - see `test/host/README.md` for the full list. In short:
it says nothing about bus timing or the ATmega, it starts from an empty state with compiled EEPROM
defaults (not the house configuration), its uptime/pseudo values are its own, and the startup throttle
ramp is reset by default. A decoder change still needs a hand-checked example in the PR.

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
