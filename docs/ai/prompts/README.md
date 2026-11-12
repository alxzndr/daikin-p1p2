# Task prompts for agents

One file per task. Hand an agent exactly one prompt, for example:

> Read `docs/ai/prompts/02-replay-harness.md` in `~/github/daikin-p1p2` and carry it out.

Every prompt sends the agent to `00-common.md` first (own worktree and branch, hard limits, evidence,
PR format).

## Order and dependencies

| # | Prompt | Branch | Needs | Touches firmware | Can run in parallel with |
|---|---|---|---|---|---|
| 01 | `01-ci-and-capture.md` | `test/ci-and-capture` | — | no | — |
| 02 | `02-replay-harness.md` | `test/replay-harness` | 01 | no (stubs only) | 01 follow-up |
| — | **Human:** record captures with `tools/capture.sh` | — | 01 | — | — |
| 03 | `03-upgrade-rc23-to-rc34.md` | `feat/upstream-rc34` | 02 + captures | version suffix only | 05 |
| 04a | `04a-mismatch-detection.md` | `feat/mismatch-detection` | 02 (03 preferred) | yes, read path | 04b, 04d, 05 |
| 04b | `04b-write-audit-log.md` | `feat/write-audit-log` | 02 (03 preferred) | yes, observe only | 04a, 04d, 05 |
| 04c | `04c-write-dedup.md` | `feat/write-dedup` | 02 + 04b, **human approval between phases** | yes, **write path** | — |
| 04d | `04d-thermal-power-and-counters.md` | `feat/thermal-power-counters` | 02 (03 preferred) | yes, read path | 04a, 04b, 05 |
| 05 | `05-decoder-study.md` | `docs/decoder-study` | 02 + captures | no | 03, 04x |

04a, 04b and 04d each add a small hook to the same big files; expect small merge conflicts and merge
them one at a time.

The ATmega firmware and the bus library are not a task: they stay untouched (rule in `00-common.md`).

## Before the first agent starts (human)

1. Create the public fork on GitHub and add it as `origin`.
2. Commit `AGENTS.md`, `CLAUDE.md` and `docs/` to `main` and push. Worktrees are created from
   `origin/main`; without this the agents do not see the rules.
3. Record the ATmega firmware version running in the house (HA device info or telnet banner) in
   `docs/ai/DAIKIN_UNIT.md`.
