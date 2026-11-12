# TESTING — without touching the heat pump

There is no simulator for a Daikin and upstream has no automated tests. Section 2 is the harness we
built for that (roadmap item #1): the real ESP decode code, compiled for the laptop, replaying captured
bus packets. Section 1 produces the input it needs.

## 1. Capture real traffic (read-only, safe)

The bridge can publish every raw packet on `P1P2/R/P1P2MQTT/bridge0`, **but the house bridge does not do
that today**: raw output is a bridge setting that is off by default (see "Raw output is off" below). Until
it is switched on, a capture holds parameters, status and discovery, and nothing the replay harness can
decode. Use the wrapper, it subscribes to `P1P2/R/#`, `P1P2/P/#` and `P1P2/S/#`, timestamps every line and
writes `captures/<date>_<time>_<label>.log`:

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
they are kept by the scrubber and carry device identifiers, so read them before committing. The replay
harness prints the same discovery payloads for a capture, keyed per entity, without a broker.

### Raw output is off on the house bridge (found 2026-11-12)

A 40-second capture on 2026-11-12 held 441 `P1P2/P` lines, 457 discovery payloads and **no `P1P2/R` line**.
The cause is the bridge's output mode (`EE.outputMode`, the `J` setting), which is the sum of bit flags:

| Bit | Effect | Source (`examples/P1P2MQTT-bridge/P1P2MQTT-bridge.ino`) |
|---|---|---|
| `0x0001` | publish raw bus packets on `P1P2/R/…` | `:4144`, `:4194` (rc23, commit `07e7230`: `:3935`, `:3985`) |
| `0x0002` | publish decoded parameters on `P1P2/P/…` | default, `INIT_OUTPUTMODE 0x0002` in `P1P2_Config.h:180` |
| `0x0004` | publish pseudo-packets on `P1P2/R/…` | `:1176` (rc23: `:1184`) |

The replay harness needs both `0x0001` and `0x0004`: pseudo-packets carry the ATmega's own state, which
the decoder also uses. Raw messages are published with QoS 0 and not retained (`P1P2_Config.h:171`, `:189`).

**Switching it on is a command to the bridge, so the human sends it — agents prepare, never send.** It
changes a setting of the ESP only; nothing is written to the P1/P2 bus or to the Daikin.

1. Read the current value (the answer arrives on `P1P2/S/P1P2MQTT/bridge0`, "Outputmode 0x… is sum of"):
   `mosquitto_pub -h <broker> -t P1P2/W/P1P2MQTT/bridge0 -m 'J'`
2. Set current value + `0x0005`. With the default `0x0002` that is `0x0007`; the argument is hexadecimal:
   `mosquitto_pub -h <broker> -t P1P2/W/P1P2MQTT/bridge0 -m 'J7'`
3. Restore: send the value read in step 1, for the default `J2`.

What to know before sending:

- **Persistence.** `J` changes the value in RAM and marks the settings as modified (`printModifyParam`
  sets `EE_dirty`). It is written to flash only when the settings are saved (`D5`, or another action that
  saves them). Not saving is the safer choice for a capture session: an ESP restart then returns to the
  old value by itself. Do not send `D5` unless raw output should stay on.
- **Load.** Every bus packet becomes one extra MQTT message. The volume on this installation has not been
  measured; measure it with the first capture (`grep -c ' P1P2/R/'`) and record it here. Both brokers
  carry it, because the first broker forwards every topic to the one on the HA host.
- **Watch afterwards:** `sensor.bridge0_bridge_error_budget` stays where it was, the bridge stays
  `online` and its uptime keeps counting (no ESP restart), parameter values keep updating.
- **Not verified:** the exact command topic on rc23 (taken from upstream `doc/P1P2MQTT.md`), and that
  setting `J` has no effect beyond the published topics. Read the `P1P2/S` answer after each command.

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
3. **Against the `P1P2/P` lines of the same file** (`--against-capture`): compares the replay with the
   parameter messages in the same input file. In a capture from the broker those are the values the
   bridge published; nothing in this repository does that, because raw output is off on the house bridge
   (section 1) and captures stay out of git - so the only file with `P` lines here is
   `test/fixtures/daikin_e_doc_packets_p_lines_handtranscribed.log`, whose `P` lines were transcribed by
   hand from the protocol tables. Expect differences by construction either way: the replay starts from
   an empty state with compiled EEPROM defaults, so it publishes each key once on first sight, and
   pseudo/uptime keys never line up. The number to look at is `final value differs`: 0 for the keys both
   sides have.

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
