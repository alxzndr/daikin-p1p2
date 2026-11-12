# Task 04d — Thermal power, COP and the frozen counter (read path)

**Branch:** `feat/thermal-power-counters` · **Depends on:** task 02 merged (task 03 preferred) ·
**Write impact:** none · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/DAIKIN_UNIT.md` (counters, typical live
values), `docs/ai/HOUSE_CONTEXT.md`, `docs/ai/ARCHITECTURE.md` ("Counters").

## Goal

Make the bridge's power and efficiency numbers explainable and consistent with what Home Assistant
calculates itself (flow × ΔT, electricity from the Shelly EM).

## Deliverables

1. **Inventory** (`docs/ai/POWER_AND_COUNTERS.md`): every power, energy and COP value the firmware
   publishes today, the formula and inputs behind it (file:line), its unit, and whether it is affected
   by the frozen `Electricity_Consumed_Compressor_Heating` counter (16 777 kWh). Include the existing
   `useTotal` option: what it does, how it is set, what changes when it is on.
2. **Thermal power from bus values**: `flow × (LWT − RWT) × c` with the constants written out
   (water at ~30 °C; state the assumption about glycol). Decide, with evidence from the captures, which
   temperature pair and flow field to use and how to treat defrost (negative ΔT), DHW runs, pump-only
   circulation and stale values.
   - If the firmware already publishes such a value: document it, compare it against the formula on
     the captures, and only fix it if it is wrong. Do not add a duplicate.
   - If you add one: new entity only, correct `device_class`/`state_class`/unit, formula in a
     host-tested module.
3. **Counter source transparency**: an entity or attribute that tells which electricity counter the
   derived values are built on (frozen heating counter or total). Changing the *default* source changes
   existing values in HA: propose it, do not do it.
4. **Unknown fields**: `DAIKIN_UNIT.md` lists unnamed entities (`unknown_buh1_q`,
   `unknown_setpoint_room_q`, `counter_unknown_00..02`) and a 111 W "gas boiler" value on a unit with
   no boiler. For each: the packet and offset, observed values in the captures, and a hypothesis
   labelled as hypothesis. Rename nothing.
5. **Tests**: host unit tests for the formula (including the edge cases above) and replay tests on the
   captures, with a comparison table against a hand calculation for three moments.

## Out of scope

- Using HA or Shelly data inside the firmware.
- Changing or removing existing entities, or their units.

## Done when

- A reader can reproduce every published power/COP number by hand from the inventory.
- All existing replay goldens are unchanged except for added keys.
- The PR separates generic commits (upstream candidates) from house-specific ones.
