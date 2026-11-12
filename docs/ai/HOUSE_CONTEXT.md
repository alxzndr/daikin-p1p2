# HOUSE CONTEXT — the one installation this fork serves first

Update this file whenever something is learned in the field. Dates are absolute.

## Hardware / network

Full unit details (models, capacities, counters, settings, field settings): `DAIKIN_UNIT.md`.


- Daikin Altherma LT EHVH08S26C?9W + ERLQ006CAV3 (E-series, P1/P2), underfloor heating, room control via the Daikin room sensor (it reads 1.3–3.4 °C high
  because it hangs above a wall tablet; HA compensates with an offset).
- 9 kW backup heater present (BUH). DHW tank (~50 °C).
- P1P2MQTT bridge (Home-CliCK v1.2): **192.168.2.110**, IoT VLAN, DHCP (no reservation yet). Telnet :23 **without lock**, web OTA :80
  **unauthenticated**. Firmware **0.9.58rc23** (compiled 2025-06-07, Daikin E-Series).
- MQTT broker: Mosquitto add-on on the Home Assistant host 10.0.1.5:1883.

## How Home Assistant uses the bridge (do not break these)

Driven by automations (EMHASS-based heating control, PV-first heat window, night setback, tests):

| Entity | Used for |
|---|---|
| `climate.bridge0_room_room_heating` | Room setpoint (real target + sensor offset; "force demand" = room + 1.5 °C) |
| `climate.bridge0_lwt_abs_heating` | Leaving-water setpoint in Abs mode (idle 30 °C, boost 38 °C, ≤ 35 on low-PV days) |
| `climate.bridge0_dhw_dhw_setpoint`, `switch.bridge0_dhw_dhw_boost` | Domestic hot water |
| `select.bridge0_quiet_quiet_mode`, `select.bridge0_quiet_quiet_level_when_on` | Quiet mode — normal value **`Always off`** |
| `sensor.bridge0_quiet_quiet_mode_active` | Confirmation that quiet mode is really off |
| `sensor.bridge0_sensors_temperature_r2t_leaving_water`, `..._r4t_return_water`, `sensor.bridge0_sensors_flow`, `..._outside`, `..._room` | Thermal power / COP calculation in HA, analysis scripts |
| `sensor.bridge0_lwt_lwt_setpoint`, `sensor.bridge0_mode_compressor_rpm` | Did the unit accept the setpoint / is it throttled |
| `number.bridge0_bridge_write_budget` (100), `..._write_budget_period` (15), `sensor.bridge0_bridge_writes_refused_budget`, `sensor.bridge0_bridge_error_budget` | Write health |
| `sensor.bridge0_meters_*` | Counters (heating electricity counter frozen at 16777) |

Electricity of the heat pump is measured by a separate Shelly EM (`sensor.shelly_em_warmtepomp_vermogen`),
which is the trusted source; the bridge's own power/COP values are secondary.

## Field incidents and known problems

- **2026-11-07 → 2026-11-12 — quiet mode stuck.** A test set quiet mode `On` (Level 2/3) at night and
  `Auto` each morning. `Quiet_Mode_Active` stayed 1 for five days (Quiet Level 3): compressor capped
  ~40 rpm, LWT 27–28 °C while 38 °C was commanded and accepted, house ~1 °C too cold. Writing
  `Always off` cleared it instantly. → Improvement: expose/alert on active-vs-commanded mismatch.
- **`writes_refused_budget` = 255** observed (counter saturates at 255?) — budget pressure from HA
  setpoint writes; HA side writes only on change with hysteresis.
- **Heating electricity counter frozen at 16777 kWh** (≈2^24 Wh) — upstream known; `useTotal` option.
- **HA log error** at every start: binary sensor `Defrost_Active_param15` published with
  `device_class: temperature` → rejected by HA (fixed upstream after rc23, issue #270/#297).
- Upstream open issues worth tracking: #274 heating on/off refused by write budget, #277 booster heater
  on/off writes EEPROM.

## Human preferences relevant to firmware work

- Comfort: rather slightly too warm than too cold. Quiet operation at night matters (neighbours).
- Prefers open source, local control, no cloud.
- Changes to the heating are made in daytime with the human present; nothing experimental before a cold
  night.
