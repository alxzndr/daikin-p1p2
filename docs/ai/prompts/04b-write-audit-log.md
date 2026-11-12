# Task 04b — Write audit log (observe writes, change none)

**Branch:** `feat/write-audit-log` · **Depends on:** task 02 merged (task 03 preferred) ·
**Write impact:** must be provably none · **Device contact:** none

Start with `docs/ai/prompts/00-common.md`. Then read `docs/ai/SAFETY.md`, `docs/ai/HOUSE_CONTEXT.md`,
`docs/ai/DAIKIN_UNIT.md` ("Write storm"), `doc/Commands-Eseries.md`, `doc/P1P2Monitor-commands.md`.

## Goal

Today a refused write is invisible: Home Assistant only sees a counter that saturates at 255. Make every
write request traceable: what was asked, by which path, whether it was passed on, whether the ATmega
accepted or refused it, and why.

## Facts to start from (verify them)

- The write budget is enforced **on the ATmega** (`examples/P1P2Monitor/P1P2Monitor.ino`, `writeBudget`,
  `writeRefusedBudget`, `writeRefusedBusy`). You may read that file; you may not change it.
- The ATmega reports refusals to the ESP as `*` text lines ("No write budget left") and through
  pseudo-packets carrying the counters. The ESP therefore has to *observe* the outcome.

## Deliverables

1. **Map of the write path** (`docs/ai/WRITE_PATH.md`): every way a write can originate on the ESP (HA
   climate/select/number/switch command topics, `P1P2/W`, telnet), the function chain to the serial
   command sent to the ATmega, the token cost, and how acceptance or refusal comes back. File:line for
   each step. This document is valuable on its own; deliver it even if the rest is blocked.
2. **Audit events** published on a new status sub-topic as one compact JSON object per write:
   origin, entity/parameter, requested value, result (`sent` / `refused_budget` / `refused_busy` /
   `rejected_invalid` / `unknown`), remaining budget when known, uptime timestamp. Bounded rate and
   fixed buffers: an audit log must never be able to flood MQTT or fragment the heap.
3. **New, self-contained module** with a host-testable core, hooked into the existing code at the
   narrowest points. The hooks only *observe*: the bytes sent to the ATmega must be identical with and
   without this change.
4. **Proof of "write impact: none"**: a host test (or instrumented stub) that feeds the same command
   sequence to the old and the new code and compares the serial output to the ATmega byte for byte.
5. Optional, only if cheap: HA discovery for "last write result" and a non-saturating refused counter.
   New entities only.

## Out of scope

- De-duplication, retries, budget changes, queueing: nothing that alters which writes reach the bus.
- Any ATmega change. If correlation of request and outcome is impossible without one, document exactly
  what is missing and stop.

## Done when

- Each origin in the write-path map produces an audit event in the host tests.
- The byte-for-byte comparison passes and is part of CI.
- Limits are stated: which outcomes can only be reported as `unknown`, and why.
