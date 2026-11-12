# DAIKIN UNIT — everything known about our heat pump

The installation this fork is developed against. Facts come from Daikin documentation, from the
bridge's own readings in Home Assistant (snapshot **2026-11-12**) and from a year of operating
notes. Mark new facts with a date and a source. Keep inferences labelled as inferences.

## Equipment

| | Model | Notes |
|---|---|---|
| Indoor unit (hydrobox + tank) | **Daikin Altherma LT EHVH08S26C(A/B)9W** (label written as `EHVH08s26c9w`; a Belgian owner of the same type reports `EHVH08S26CA9W`) | Floor-standing, heating only (EHV**H**), 8 kW class, **integrated 260 L DHW tank**, **9 W** = 9 kW backup heater (BUH, 3 + 6 kW in two steps). The BUH is also the DHW booster (tank type [E-07]=1). Exact generation (CA vs CB) is **not verified** — read the label before trusting generation-specific codes. |
| Outdoor unit | **Daikin ERLQ006CAV3** | Low-temperature split, **6 kW**, 1-phase (V3), **R410A**. Databook (EEDEN16): **5.31 kW at A-7/W35, COP 2.84**; compressor electrical ~1.9 kW at A-7/W35, ~2.3 kW at W45. |
| Emitters | Underfloor heating | Low-temperature system; Daikin's design case 35/30 °C. |
| User interface (MMI) | Daikin wired controller on the indoor unit, plus the Daikin room sensor/controller in the living room | Installer menu was **not accessible** to the owner (2026-03). The room sensor hangs directly **above a wall tablet** and reads **1.3–3.4 °C too high** (see below). |
| Bus | P1/P2 (Daikin **E-series** protocol, `-D E_SERIES`) | Bridge claims auxiliary controller slot **0xF0**. |
| Bridge | Home-CliCK **P1P2MQTT bridge**, firmware **0.9.58rc23** (2025-06-07, Daikin E-Series) | 192.168.2.110 (IoT VLAN). ATmega firmware version: **to be recorded** before the first flash. |
| Second reader | **ESPAltherma** on an ESP32 at 10.0.1.155 (serial X10A port), 5 summary sensors in HA (`espaltherma_*`) | Independent read-only source, useful to cross-check bridge decoding. |
| Electricity meter for the heat pump | Shelly EM channel `sensor.shelly_em_warmtepomp_vermogen` | **Trusted** source for heat-pump electricity. Bridge counters are secondary (see frozen counter). |
| House | Belgium; design outdoor temp about −7 °C | Grid tariff with a monthly **capacity charge** (peak kW): a 9 kW BUH quarter-hour is very expensive. PV 21 kWp + 27.7 kWh home battery; the heat pump is scheduled to run on PV. |

## Lifetime counters (bridge, 2026-11-12)

| Counter | Value |
|---|---|
| Heat produced, space heating | 80 922 kWh |
| Heat produced, DHW | 21 536 kWh |
| Heat produced, total | 102 458 kWh |
| Electricity consumed, total | 39 336 kWh |
| Electricity, compressor heating | **16 777 kWh — FROZEN** (≈ 2^24 Wh; known upstream, `useTotal` option) |
| Electricity, compressor DHW | 4 891 kWh |
| Electricity, BUH heating / BUH DHW | **9 490 / 7 778 kWh** (≈ 44 % of all electricity: the main efficiency problem) |
| BUH hours: step 1 heating / DHW, step 2 heating / DHW | 1 062 / 182 h, 638 / 831 h |
| Compressor hours: heating / DHW | 20 615 / 2 535 h |
| Compressor starts | 32 901 (≈ 42 min average run) |
| Circulation pump hours | 25 737 h |
| Bridge "COP lifetime" | 4.82 (unreliable: built on the frozen counter) |

Counters are whole kWh. They update about hourly and only while `Counter_Request_Function` (C2) is on.

## Operating mode and settings as run today

| Setting | Value | How it is set |
|---|---|---|
| LWT setpoint mode | **Abs** (fixed), switched from WD on 2026-03-28 | `select.bridge0_mode_program_wd_abs`. The WD curve gave ~26 °C at mild temperatures, too low for the comfort target. |
| LWT setpoint | 30 °C idle, **38 °C boost** on normal days, ≤ 35 °C on low-PV days | HA writes `climate.bridge0_lwt_abs_heating` (absolute value in Abs mode). Deviation currently 0. |
| Room setpoint | real target + sensor offset; "force demand" = Daikin room + 1.5 °C | `climate.bridge0_room_room_heating`. The unit runs in **room-thermostat control**: it needs ~1 °C under setpoint to start (hysteresis). |
| Real comfort targets | idle 21.5 °C, boost 23.5 °C, night floor 20 °C (22:00 → start of the sun window) | HA helpers |
| When it runs | Only inside a daily **sun window** derived from the Solcast forecast (≥ 1.5 kW; 8–11 h to 13–17 h, at least 3 h) | HA automation. Owner's rule: **no loud night runs** (noise for family and neighbours, poor COP). |
| DHW | Baseline 37 °C; boosts on PV surplus (60 °C), cheap price (45 °C, only by day), safety (< 35/40 °C), winter evening reserve 50 °C | `climate.bridge0_dhw_dhw_setpoint`, `switch.bridge0_dhw_dhw_boost`. Tank was 49.4 °C on 2026-11-12. |
| Quiet mode | **Always off** (level when on: Level 1) | `select.bridge0_quiet_quiet_mode`. See incident below. |
| Preset mode | Bridge reads **`Schedule`** on 2026-11-12, although it was set to Comfort at the MMI on 2026-03-10 | **Inconsistent, to investigate**: a schedule on the MMI can override HA setpoints. Preset temps read Comfort 23 / Eco 23. |
| Write budget | 100 tokens, 1 per 15 min | `number.bridge0_bridge_write_budget(_period)`. On/off actions cost 3 tokens. **Rule in HA: write only when the value differs.** |
| Error budget | 20 (healthy) | `sensor.bridge0_bridge_error_budget` |
| Typical live values (2026-11-12, 3.5 °C outside) | LWT 29.9, RWT 24.9, flow 16.9 l/min, compressor 73 rpm-units, ~5.8 kW heat | — |

### Field settings (MMI, installer level)

- The bridge can **write** only five field settings: [C-07] control method, RT modulation on/off, RT
  modulation max, [7-02], [9-04]. Each change needs a Daikin restart. Everything else ([4-00], [5-01],
  [1-0x], [9-09] …) is readable under `HC_FieldSettings` only.
- Current values have **not yet been read out systematically**. Reading them (decoding the field-setting
  packets) is a good first task for the fork. [C-07] is believed to be 2 (room thermostat), given the
  observed behaviour.
- Recommended values from the 2026-10 analysis (a report in the private HA project notes),
  **not applied yet**:

| Code | Meaning | Factory | Recommended |
|---|---|---|---|
| [5-00] / [5-01] | BUH allowed above equilibrium / equilibrium temp | ? / 0 °C | 1 (not allowed) / −5 °C |
| [5-03] | BUH DHW assist below outdoor temp | 0 °C | −5 °C |
| [4-00] | BUH operation | 1 | 1 now, 2 (DHW only) once the heat-loss check confirms ≤ ~195 W/K |
| [4-07] | Second BUH step | — | 0 (max 3 kW) if DHW ≤ 48 °C |
| [4-08] / [5-09] | Power limitation | — | 1 / ≈ 4 kW |
| [9-04] | Overshoot | 1 °C | 3 °C (fewer cycles) |
| [9-01] | Min LWT heating | 25 °C | **keep ≥ 25 °C**: below it the compressor will not start and the BUH takes over |
| [9-00] | Max LWT heating | 55 °C | 40 °C (floor heating) |
| [1-00..1-03] | Weather curve | −10/35, 15/25 | −7 → 33 °C, 15 → 25 °C |
| [4-02] | Heating stop temperature | 25 °C | 16 °C |
| DHW | Setpoint | — | ≤ 48 °C (the compressor maxes at 55 °C, 50 °C in cold weather; above that the BUH does the rest) |

Weekly **disinfection** (60 °C, 40 min on CB) is by definition electric (BUH).

## Known behaviour and quirks (field evidence)

- **BUH is the main cost.** It is used structurally for defrost with cold return water (< ~25 °C,
  typical with floor heating), for start-up below 15 °C water temperature, and for DHW above the
  compressor ceiling.
- **Quiet mode stuck, 2026-11-07 → 11-12.** "On" was set at night and "Auto" in the morning, but
  `Quiet_Mode_Active` stayed 1 at Level 3 for five days. The compressor was capped (~40), LWT stayed at
  27–28 °C while 38 °C was accepted. Writing **"Always off"** cleared it instantly. Quiet level 3
  roughly halves maximum power.
- **Room sensor offset.** The Daikin room sensor reads 1.3–3.4 °C high. HA corrects with
  `sensor.daikin_room_sensor_offset`: a 30-minute average of Daikin room minus an independent living-room
  sensor, added to the targets and written only on a change > 0.4 °C. A real fix at the MMI is pending.
- **Room-thermostat hysteresis.** A boost with the setpoint barely above the (inflated) room reading
  does not start the compressor (seen 2026-10-16/20). Hence "force demand" = room + 1.5 °C.
- **Short cycling history.** 94 starts on 2026-03-09 (2 min on / 6 min off). The fix was MMI changes
  plus Abs mode.
- **Write storm, 2026-08-30 → 09-09.** Unconditional HA writes every 5 minutes drained the budget,
  refused-counter saturated at 255 and writes silently dropped. Fixed on the HA side.
- **Write latency.** Commands take seconds to reach the bus; readback can lag 10–30 s.
- **Discovery error.** `Defrost_Active_param15` is published with `device_class: temperature`, and
  HA rejects it at every start (fixed upstream after rc23).
- **Unknown fields worth decoding.** These exist in HA but are unnamed: `binary_sensor.bridge0_unknown_buh1_q`
  (on), `sensor.bridge0_unknown_setpoint_room_q` (30.0), `sensor.bridge0_meters_counter_unknown_00..02`.
  A gas-boiler production of 111 W is shown although there is no boiler: probably a decoding artefact.

## Owner's goals for the unit (drive the roadmap)

1. Run on PV, by day; nothing loud at night.
2. Cut BUH use without losing comfort (slightly too warm is better than too cold).
3. Never cause a capacity-tariff peak.
4. Know the true state of the unit (what it actually does, not just what was commanded).

## Sources

- Daikin installer reference guides for ERLQ-CV3 / EHVH-CB and EHVH-CA, and databook EEDEN16 (links in
  the HA-repo report above).
- P1P2MQTT discussions #135, #137, #143, #181, #204; issues #270, #274, #277, #297.
- Private HA project notes (optimisation reports, MMI change checklist 2026-03-10).
