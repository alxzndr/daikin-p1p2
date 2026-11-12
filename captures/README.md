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
```

`tools/capture.sh` only subscribes (`mosquitto_sub -t 'P1P2/R/#' -t 'P1P2/P/#'
-t 'P1P2/S/#'`). It has no publish in it and never touches `P1P2/W*`. Stop a running
capture with Ctrl-C.

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

## Scrubbing

```bash
tools/scrub_capture.sh captures/<file>.log          # writes <file>_scrubbed.log
tools/scrub_capture.sh -o /tmp/out captures/*.log   # or somewhere else
```

The scrubbed copy keeps only `#` header lines, `P1P2/R` and `P1P2/P` lines, and drops:

* every `P1P2/S` line;
* every line on any other topic;
* every line containing a credential-like word (`password`, `passwd`, `pwd`, `psk`,
  `secret`, `token`, `api_key`, `credential`, `private_key`, `bearer`, `auth`, `ssid`,
  `wlan`, `wifi`, `-P `, `--user`), listed in the script;
* the broker address in the header (replaced by `<redacted>`).

Every drop is printed with file, line number and reason, so you can see that nothing
important went away. The script exits non-zero if a `P1P2/S` line survives. Reading
the scrubbed file yourself before committing stays your job - a script cannot judge
what is personal.

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

Winter situations (`defrost`, `buh_active`, `night_idle`) cannot be captured in
summer - capture what the unit actually does now and add the cold-weather ones when
they happen.

## Before capturing

* The Mac must reach the broker (`10.0.1.5:1883`) - on the LAN, not over the guest VLAN.
* Check the bridge is publishing: `P1P2/L/P1P2MQTT/bridge0` = `online`, or look at HA.
* `brew install mosquitto` if `mosquitto_sub` is missing.
* A capture of a live house is anonymous bus data, but it does contain your setpoints,
  temperatures and switch-on moments: commit only the scrubbed file, and only when you
  are fine with that being on public GitHub.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `mosquitto_sub: Connection refused` / timeout | Wrong host, or you are not on the network that reaches 10.0.1.5 |
| `Error: Bad file descriptor` | That is what the Homebrew build of mosquitto_sub prints when it cannot connect at all - wrong port, broker down, or firewall |
| `Module function not supported` / `Bad user name or password` | Broker credentials in `tools/capture.env` are wrong or empty |
| 0 lines captured | Bridge offline (`P1P2/L/...` not `online`), or MQTT integration restarted; check HA |
| huge file | `P1P2/S` is chatty when the bridge runs in verbose mode - scrubbing removes it, and `gzip` is allowed for committed files |
