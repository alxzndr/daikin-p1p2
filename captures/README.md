# `captures/` — recorded P1/P2 bus traffic

Raw packets recorded from the bridge's MQTT topics, used as input for the offline
decode tests (task 02, `docs/ai/TESTING.md` §2). Capturing is **read-only**: nothing
that happens in this folder can change what the heat pump does.

## Quick start

```bash
cp tools/capture.env.example tools/capture.env    # then edit host/user/password
tools/capture.sh -d 900 defrost                   # 15 minutes, stops by itself
tools/scrub_capture.sh captures/2026-11-14_203112_defrost.log
git add captures/2026-11-14_203112_defrost_scrubbed.log

tools/capture.sh -D -d 30 contract                # also the HA discovery payloads (task 03)
```

`tools/capture.sh` only subscribes (`mosquitto_sub -t 'P1P2/R/#' -t 'P1P2/P/#'
-t 'P1P2/S/#'`). It has no publish in it and never touches `P1P2/W*`. Stop a running
capture with Ctrl-C.

### The `-D` option (Home Assistant discovery payloads)

`-D` adds one subscription per entity component the bridge uses, fixed to this bridge:
`<prefix>/<component>/<node_id>/+/config` with `prefix=homeassistant`, `node_id=bridge0`
and `component` in `sensor`, `binary_sensor`, `number`, `switch`, `button`, `select`,
`text`, `climate` - the eight entries of `haPrefixString[]` in
`examples/P1P2MQTT-bridge/P1P2_HomeAssistant.h`, assembled by `HACONFIGTOPIC()` in
`P1P2MQTT-bridge.ino` as `<prefix>/<component>/<EE.bridgeName>/<object_id>/config`.
Other devices in the house (ESPAltherma, Shelly, …) publish under a different node id,
so they cannot end up in the capture. If you ever rename the bridge or change the
discovery prefix, set `P1P2_MQTT_HA_NODE_ID` / `P1P2_MQTT_HA_DISCOVERY_PREFIX` (in
`tools/capture.env`) - `tools/scrub_capture.sh` reads the same two names.

Discovery payloads are **retained**, so they all arrive the moment the capture
subscribes: `-d 30` is enough, no bridge restart and no `MQTT_Rebuild` needed. Take one
with `-D` before and after a firmware upgrade - that is what task 03 compares (entity
names, `uniq_id`s, device classes, units, `sw`).

## File names

```
captures/YYYY-MM-DD_HHMMSS_<label>.log            raw, never committed
captures/YYYY-MM-DD_HHMMSS_<label>_scrubbed.log   safe to commit
```

`captures/.gitignore` ignores everything in this folder except `README.md`,
`.gitignore` and `*_scrubbed.log` / `*_scrubbed.log.gz`. A raw file cannot be added
by accident; `git add captures/<file>.log` prints "ignored" and does nothing.

## Line format

```
# captured with tools/capture.sh on 2026-11-14T19:02:11Z
# broker: <redacted>
# topics: P1P2/R/# P1P2/P/# P1P2/S/#
# label: defrost
2026-11-14T20:31:10 1 P1P2/P/P1P2MQTT/bridge0/T/R2T/Leaving_Water_Temperature 31.2
2026-11-14T20:31:12 0 P1P2/R/P1P2MQTT/bridge0 000012010000f03d...
2026-11-14T20:31:12 0 P1P2/P/P1P2MQTT/bridge0/T/R2T/Leaving_Water_Temperature 31.4
2026-11-14T20:31:09 1 homeassistant/sensor/bridge0/<object_id>/config {"name":"...","uniq_id":"...","dev":{...}}
```

* One message per line, four fields: local timestamp `YYYY-MM-DDTHH:MM:SS`, retained flag, topic,
  payload. The payload is everything after the third space and may itself contain spaces.
* The retained flag is `1` for a value the broker had stored and replays when the capture
  subscribes (the state from before the capture, all stamped with the start time), `0` for a
  message published while the capture ran. Treat `1` lines as initial state, not as events.
* Lines starting with `#` are comments from the tool - skip them when parsing.
* `P1P2/R/P1P2MQTT/bridge0` carries the raw hex packets (plus the bridge's own
  pseudo-packets). That is the input for the replay harness.
* `P1P2/P/...` carries what the bridge decoded and published; the replay harness
  compares its own decode against these lines.
* `P1P2/S/...` is verbose bridge status. It is captured (useful while debugging) but
  **scrubbing removes it**, because config values can show up there.
* `homeassistant/<component>/bridge0/<object_id>/config` (only with `-D`) is the Home
  Assistant discovery payload of one entity, always with retained flag `1`. Its payload
  is one line of JSON, so the whole entity definition is on that single line.

## Scrubbing

```bash
tools/scrub_capture.sh captures/<file>.log          # writes <file>_scrubbed.log
tools/scrub_capture.sh -o /tmp/out captures/*.log   # or somewhere else
```

The scrubbed copy keeps `#` header lines, `P1P2/R` lines, `P1P2/P` lines, and the
discovery `config` lines of **this** bridge (`.../bridge0/<object_id>/config`), and drops:

* every `P1P2/S` line;
* discovery payloads of other devices (`homeassistant/<component>/<other-node>/...`) and
  discovery-shaped topics that do not match `<prefix>/<component>/<node>/<object>/config`;
* every line on any other topic;
* every line containing a credential-like word (`password`, `passwd`, `pwd`, `psk`,
  `secret`, `token`, `api_key`, `credential`, `private_key`, `bearer`, `auth`, `ssid`,
  `wlan`, `wifi`, `-P `, `--user`), listed in the script;
* the broker address in the header (replaced by `<redacted>`).

Every drop is printed with file, line number and reason, so you can see that nothing
important went away. The script exits non-zero if a `P1P2/S` line survives. Reading
the scrubbed file yourself before committing stays your job - a script cannot judge
what is personal.

**Discovery lines are extra work for you.** They survive on purpose (task 03 needs them),
but they carry device identifiers: `uniq_id`, `dev.ids`/`dev.name`, entity names, the
availability topics and the firmware version. The script says so when it kept any -
read those lines before you commit the file.

Note: patterns are chosen so that no real parameter key matches. `ota` is *not* in the
list because it would drop every `*_Total` counter; the OTA password only ever appears
on `P1P2/S`, which is dropped as a whole.

## Situations worth capturing

Longer is better (the replay harness needs the packets *before* and *after* an event,
not just the middle), and note in the PR what HA was commanding at the time.

| Label | What to do | Suggested `-d` |
|---|---|---|
| `startup` | Bridge or heat pump just powered on; the whole discovery/handshake | 600 |
| `defrost` | Let it run through a real defrost cycle, from before it starts to a few minutes after | 1800 |
| `dhw_run` | Start a DHW heating run (`dhw_boost` or raise the DHW setpoint), capture to cut-off | 2700 |
| `buh_active` | A moment where the 9 kW backup heater is really on | 1200 |
| `quiet_on` | Command quiet mode `On`, then `Always off` again (daytime, one write, see SAFETY.md) | 900 |
| `quiet_auto_stuck` | The 2026-11 incident: `Auto` while `Quiet_Mode_Active` stays 1 | 1800 |
| `night_idle` | Night, heating idle, no writes: baseline traffic and idle LWT behaviour | 3600 |
| `boost_lwt38` | Low-PV day: LWT setpoint from 30 °C to 38 °C and back, watch acceptance | 1200 |
| `compressor_throttle` | Any period where `compressor_rpm` is clearly capped | 1200 |
| `contract` (`-D`) | Not a bus situation: the HA entity contract of the firmware that is running now, before/after a firmware change (task 03) | 30 |

Winter situations (`defrost`, `buh_active`, `night_idle`) cannot be captured in
summer - capture what the unit actually does now and add the cold-weather ones when
they happen.

## Before capturing

* The Mac must reach the broker the bridge publishes to (`10.0.1.10:1883`) - on the LAN, not over
  the guest VLAN.
* **Raw packets are not published by default.** If a capture has no `P1P2/R` line, raw output is
  off on the bridge; `docs/ai/TESTING.md` section 1 explains the setting and the command the human
  sends to switch it on. Without `P1P2/R` lines the replay harness has nothing to decode.
* Check the bridge is publishing: `P1P2/L/P1P2MQTT/bridge0` = `online`, or look at HA.
* `brew install mosquitto` if `mosquitto_sub` is missing.
* A capture of a live house is anonymous bus data, but it does contain your setpoints,
  temperatures and switch-on moments: commit only the scrubbed file, and only when you
  are fine with that being on public GitHub.
* With `-D` it is no longer only bus data: discovery payloads name your entities and the
  device identifiers of the bridge. Same rule - scrubbed only, and read before pushing.

## Replaying a capture

Nothing in this directory is executed by the tests: `captures/*.log` stays out of git, so
CI only replays the committed fixtures in `test/fixtures/`. On your own machine:

```bash
test/run_replay.sh captures/<file>_scrubbed.log                    # golden + hand-checked values
test/run_replay.sh captures/<file>_scrubbed.log --against-capture  # vs the bridge's own P1P2/P lines
test/run_replay.sh captures/<file>.log                             # unscrubbed works too
```

It compiles the ESP firmware sources for your own machine (nothing is flashed, no broker
is contacted) and feeds them the `P1P2/R` lines from the file. See `docs/ai/TESTING.md` §2 and `test/host/README.md`
for what the result does and does not prove.

If you want to commit a capture as a regression fixture, scrub it first
(`tools/scrub_capture.sh`), put it in `test/fixtures/`, then generate the reference:
`test/run_replay.sh test/fixtures/<name>.log --update-golden` and review the golden file
line by line before committing it.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `mosquitto_sub: Connection refused` / timeout | Wrong host, or you are not on the network that reaches the broker |
| `Error: Bad file descriptor` | That is what the Homebrew build of mosquitto_sub prints when it cannot connect at all - wrong port, broker down, or firewall |
| `Module function not supported` / `Bad user name or password` | Broker credentials in `tools/capture.env` are wrong or empty |
| 0 lines captured | Bridge offline (`P1P2/L/...` not `online`), or MQTT integration restarted; check HA |
| huge file | `P1P2/S` is chatty when the bridge runs in verbose mode - scrubbing removes it, and `gzip` is allowed for committed files |
