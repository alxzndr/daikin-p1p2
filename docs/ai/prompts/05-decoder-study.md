# Task 05 — Study: a table-driven E-series decoder (research, no firmware change)

**Branch:** `docs/decoder-study` · **Depends on:** task 02 merged, house captures available ·
**Write impact:** none · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/ARCHITECTURE.md`, `docs/ai/TESTING.md`,
`doc/LogicalFormat/README-Daikin.md` and the E-series protocol files next to it.

## Goal

`examples/P1P2MQTT-bridge/P1P2_ParameterConversion.h` is 6.4k lines of nested `switch` statements and
macros mixing decoding, state keeping, publishing and HA discovery. The human will decide **later**
whether to replace the E-series part with a data-driven decoder. Give them the facts for that decision.
Nothing in this task may end up in a flashed binary.

## Deliverables — one document, `docs/ai/design/decoder-study.md`

1. **How the current decoder works**: the macros (`PARAM_KEY`, `FIELDKEY`, `PARAM_VALUE_*`, `value_*`
   helpers…), the "seen/changed" bookkeeping, where state is kept (`M.R.*`, RTC memory), where
   publishing and discovery are triggered, and the side effects hidden inside decode cases (for example
   a case that also updates a mode variable or triggers a pseudo-packet). File:line throughout.
2. **Inventory for the E-series**: number of packet types, fields and parameters; how many are pure
   "offset → conversion → key" (table material) and how many carry logic or side effects (list these).
3. **Coverage from the house**: using the replay harness, which decode cases the house captures
   actually exercise, and which are never hit by our unit.
4. **Options**, each with effort, risk, flash/RAM effect and upstream-ability:
   - A. leave as is, only add tests;
   - B. extract the pure cases into generated tables inside the same file, keep the special cases;
   - C. a new E-series-only decoder, validated key-by-key against the old one in the replay harness;
   - D. decode on the host (HA side) from the raw `P1P2/R` topic and keep the firmware as a transport.
5. **Proof of concept, host only** (under `test/host/experiments/`, not compiled into firmware):
   decode **one** packet type with a table and show identical output to the old decoder on the captures.
   Report lines of code and estimated flash/RAM against the current implementation.
6. **License note**: a table derived from this file is a derivative of CC BY-NC-ND material and stays
   in the public fork under the same license. Say what that means for each option.
7. **Recommendation** with the conditions under which you would change it, and the first three
   concrete steps if the human says yes.

## Out of scope

- Any change to firmware sources or to what is published.
- F-series and other brands, beyond noting how each option affects "keep them compiling".

## Done when

- Numbers are measured (from the harness or a script committed under `tools/`), not estimated by eye.
- The proof of concept runs in CI as a host test and is clearly marked experimental.
