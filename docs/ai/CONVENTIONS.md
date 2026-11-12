# CONVENTIONS

## License and attribution (non-negotiable)

- Project license: **CC BY-NC-ND 4.0 + exceptions** (`LICENSE.md`). Our modifications may be distributed
  **only as this public GitHub fork**, same license, non-commercial, with attribution.
- Keep all existing headers and the AltSoftSerial MIT notice in `P1P2MQTT.cpp`.
- New files: add a short header stating they are part of the P1P2MQTT fork under the project license.
- Do not vendor code with incompatible licenses (GPL-only, proprietary). Ask first for any new dependency.
- Contributions offered upstream are, per `CONTRIBUTING.md`, deemed to waive NC/ND towards the project
  owner — the human decides what goes upstream.

## Git

- Remotes: `upstream` = Arnold-n/P1P2MQTT, `origin` = our public fork (once created).
- `main` tracks upstream + our merged work. One branch per change: `fix/…`, `feat/…`, `docs/…`, `test/…`.
- Rebase on `upstream/main` regularly; keep our changes as small, separable commits so they can be
  offered upstream individually.
- Commit messages in English, imperative, with *why*. Reference upstream issues (`Arnold-n/P1P2MQTT#277`).
- Never commit captures containing credentials, `.pio/` build output, or device-specific secrets.

## Versioning

- Keep upstream's version numbers and append a fork suffix: `0.9.58rc34-ag.N`
  (`SW_*_VERSION` + the rc string in `P1P2_Config.h` / `P1P2Config.h`). HA shows it as `sw_version`.
- Every binary that may be flashed gets a new N and a line in `docs/ai/CHANGELOG.md` (create on first use):
  version, date, what changed, write-impact (yes/no), tested how.

## Code style

- Match the surrounding style of the file you edit (indentation, brace style, naming). Do not reformat.
- C++ for ESP8266 Arduino core 3.0.2: avoid `String` churn and heap fragmentation in hot paths; prefer
  `F()`/PROGMEM for constant strings; no dynamic allocation in packet handling.
- ATmega: no blocking code in the bus path; no new interrupts; RAM is 2 KB.
- Compile-time switches follow the existing `#ifdef E_SERIES` pattern; keep other series building.
- Comments explain bus semantics with the packet evidence (`000012 … byte 7 bit 3 = quiet active`).

## Pull requests (even when merging to our own main)

Template:
- **What & why**
- **Write impact**: none / describe (packet, frequency, EEPROM, inverse, confirmation field)
- **HA contract impact**: none / entities/topics/units changed
- **Tests**: build envs run, offline replay results, hand-checked packet examples
- **Flash plan** (if relevant): binary + SHA-256, command, rollback, post-flash checks
