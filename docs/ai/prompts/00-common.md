# Common rules for every task prompt in this folder

Every prompt in `docs/ai/prompts/` starts by sending you here. These rules apply on top of `AGENTS.md`.

## 1. Read first

1. `AGENTS.md` (repo root), fully.
2. The `docs/ai/` files your prompt lists.
3. Your prompt, fully, including "Out of scope" and "Done when".

## 2. Work in your own worktree, on your own branch

Never work in the main checkout and never commit to `main`. Other agents are working in parallel.

```bash
cd ~/github/daikin-p1p2
git fetch origin
git worktree add ../daikin-p1p2-wt/<branch> -b <branch> origin/main
cd ../daikin-p1p2-wt/<branch>
```

- `<branch>` is given in your prompt. All edits, builds and commits happen inside that worktree.
- `origin` is our public fork, `upstream` is Arnold-n/P1P2MQTT. Never push to `upstream`.
- If your prompt names a branch it depends on and that branch is not merged into `origin/main` yet,
  stop and say so. Do not copy another agent's unmerged work.
- Finish with: commits on your branch, `git push -u origin <branch>`, and a draft PR **against our fork's
  `main`** (`gh pr create --draft --repo <our fork> --base main`). Do not merge. Do not remove the worktree.

## 3. Hard limits (no exceptions, no "just this once")

- No flashing, no OTA, no telnet, no `mosquitto_pub` to `P1P2/W*`, no HA service calls that write.
  The bridge at 192.168.2.110 is not to be contacted at all.
- Do not edit `P1P2MQTT.cpp`, `P1P2MQTT.h` or anything under `examples/P1P2Monitor/`. The ATmega side
  stays byte-identical to upstream.
- Do not change the ESP8266 platform, core or library versions in `platformio.ini`.
- Do not rename or change any MQTT topic, key, unique_id, unit or discovery payload unless your prompt
  says so explicitly.
- Keep every license header. New files get a short header: part of the P1P2MQTT fork, CC BY-NC-ND 4.0
  with the exceptions in `LICENSE.md`.
- No secrets in the repo (MQTT/WiFi/OTA credentials, tokens). Captures are checked for `P1P2/S` lines
  that could contain them before they are committed.

## 4. Upstream-friendliness

We want to offer generic fixes back to Arnold-n/P1P2MQTT.

- Keep generic changes (useful for every user) and house-specific changes in **separate commits**, and
  say in the PR which is which.
- Minimal diffs in the big upstream files; match the surrounding style; no reformatting.
- Fork-only material lives in new paths (`tools/`, `test/`, `docs/ai/`, `.github/workflows/`), so it
  never conflicts with an upstream rebase.

## 5. Evidence, not claims

- Every code change: `pio run -e Daikin-E-USB` in `examples/P1P2MQTT-bridge/`, and `Daikin-F-USB` if you
  touched code shared between series. Report the exact command, result, and flash/RAM usage before/after.
- Anything you could not run or verify is listed as such in the PR. "Should work" is not accepted.
- If the task turns out to need something outside its scope (a write path, an ATmega change, a contract
  change), stop and report instead of doing it.

## 6. PR description

Use the template from `docs/ai/CONVENTIONS.md` (What & why / Write impact / HA contract impact / Tests /
Flash plan), plus: **Upstream candidate: yes/no, which commits**, and **Not verified**.
