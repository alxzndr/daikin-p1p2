#!/usr/bin/env python3
"""compare.py — turn two replay outputs into a readable difference report.

Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.

Two modes:

  golden   compare a replay run against the stored golden file
           (what this decoder produced when the golden was accepted).
           Differences are reported per topic with old/new payload; a pure timing
           difference is reported separately and does not fail the run.

  capture  compare a replay run against the P1P2/P lines of the same capture, i.e.
           against what the bridge in the house actually published. Differences that
           are expected by construction (publish-on-change, pseudo packets, ESP
           uptime, the birth message) are grouped and explained, see --help.
"""

import argparse
import difflib
import os
import sys

COLUMNS = 5  # <vsec> <channel> <retain> <topic> <payload...>


def fail(msg):
    print(msg, file=sys.stderr)
    return 1


def read_replay_lines(path):
    """Yield (vsec:int, channel:str, retain:str, topic:str, payload:str)."""
    out = []
    with open(path, "r", errors="replace") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            parts = line.split(" ", COLUMNS - 1)
            if len(parts) < COLUMNS - 1:
                print(f"warning: {path}:{n}: not a replay line: {line[:60]}", file=sys.stderr)
                continue
            while len(parts) < COLUMNS:
                parts.append("")
            try:
                vsec = int(parts[0])
            except ValueError:
                print(f"warning: {path}:{n}: bad virtual time: {line[:60]}", file=sys.stderr)
                continue
            out.append((vsec, parts[1], parts[2], parts[3], parts[4]))
    return out


def read_capture_p_lines(path):
    """Return (initial, events) for P1P2/P lines: key -> list of (vsec_or_ts, value).

    retained == 1 means "state from before the capture", so those go to initial.
    """
    initial, events = {}, []
    with open(path, "r", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            parts = line.split(" ", 3)
            if len(parts) < 4:
                continue
            ts, flag, topic, payload = parts
            if not topic.startswith("P1P2/P/"):
                continue
            key = key_of(topic)
            if flag == "1":
                initial[key] = payload
            else:
                events.append((ts, key, payload))
    return initial, events


def key_of(topic):
    """Everything after P1P2/P/<device>/<bridge>/ so replay and capture line up."""
    seg = topic.split("/")
    # P1P2 / P / <device> / <bridge> / <category> / <src> / <KEY>
    if len(seg) >= 5 and seg[0] == "P1P2" and seg[1] == "P":
        return "/".join(seg[4:])
    return topic


# ================================================================== golden ====


def cmd_golden(args):
    got = read_replay_lines(args.replay)
    if not os.path.exists(args.golden):
        if args.update:
            want = []
        else:
            return fail(f"no golden file at {args.golden}\n"
                        f"  create it with: test/run_replay.sh {args.capture or '<capture>'} --update-golden")
    else:
        want = read_replay_lines(args.golden)

    got_content = [(c, r, t, p) for (_v, c, r, t, p) in got]
    want_content = [(c, r, t, p) for (_v, c, r, t, p) in want]

    if args.update:
        with open(args.golden, "w") as f:
            f.write("# golden file for the offline replay harness - test/run_replay.sh --update-golden\n")
            f.write("# columns: <virtual seconds> <channel> <retain> <topic> <payload>\n")
            for (v, c, r, t, p) in got:
                f.write(f"{v} {c} {r} {t} {p}\n")
        print(f"golden written: {args.golden} ({len(got)} messages)")
        return 0

    if got_content == want_content:
        timing_only = [g for g, w in zip(got, want) if g[0] != w[0]]
        print(f"golden OK: {len(got)} messages identical")
        if timing_only:
            print(f"  note: {len(timing_only)} message(s) published at another virtual second (not a failure)")
        return 0

    diff = difflib.unified_diff(
        [f"{c} {t} {p}" for (c, r, t, p) in want_content],
        [f"{c} {t} {p}" for (c, r, t, p) in got_content],
        fromfile=f"{args.golden} (golden)",
        tofile=f"{args.replay} (this run)",
        lineterm="",
        n=2,
    )
    changed = 0
    for line in diff:
        if line.startswith(("+", "-")) and not line.startswith(("+++", "---")):
            changed += 1
        print(line)
    print(f"\nGOLDEN MISMATCH: {changed} changed/added/removed message line(s); "
          f"golden {len(want_content)} vs run {len(got_content)}")
    return 1


# ================================================================== capture ====


def cmd_capture(args):
    got = read_replay_lines(args.replay)
    initial, events = read_capture_p_lines(args.capture)

    replay = {}
    order = []
    for (v, c, _r, t, p) in got:
        if c != "P":
            continue
        k = key_of(t)
        if k not in replay:
            replay[k] = []
            order.append(k)
        replay[k].append((v, p))

    capture = {}
    for (ts, k, p) in events:
        capture.setdefault(k, []).append((ts, p))

    both, mismatch, only_replay, only_capture = [], [], [], []
    for k in replay:
        if k in capture:
            both.append(k)
            if replay[k][-1][1] != capture[k][-1][1]:
                mismatch.append(k)
    only_replay = [k for k in replay if k not in capture]
    only_capture = [k for k in capture if k not in replay]

    print(f"capture P events: {len(events)} on {len(capture)} keys "
          f"({len(initial)} keys already known from retained state)")
    print(f"replay  P events: {sum(len(v) for v in replay.values())} on {len(replay)} keys")
    print(f"keys in both: {len(both)}, of which final value differs: {len(mismatch)}")
    print(f"keys only in replay:  {len(only_replay)}")
    print(f"keys only in capture: {len(only_capture)}")

    if mismatch:
        print("\nVALUE MISMATCH (same key, different value)")
        print(f"{'key':<48} {'capture':>16} {'replay':>16}")
        for k in sorted(mismatch):
            print(f"{k:<48} {capture[k][-1][1]:>16} {replay[k][-1][1]:>16}")

    if only_replay:
        print("\nONLY IN REPLAY - the bridge did not publish these during the window")
        print("  expected causes: the replay starts from an empty state, so every key is")
        print("  'changed' the first time it is seen; ESP/ATmega pseudo packets carry a")
        print("  different uptime; keys the bridge published before the capture started.")
        for k in sorted(only_replay):
            print(f"  {k:<52} first={replay[k][0][1][:16]:<16} last={replay[k][-1][1][:16]}")

    if only_capture:
        print("\nONLY IN CAPTURE - the replay never produced these")
        print("  expected causes: counters (only with Counter_Request_Function on),")
        print("  parameters whose packets were skipped by throttling, values the bridge")
        print("  published before the capture and that did not change again.")
        for k in sorted(only_capture):
            print(f"  {k:<52} last={capture[k][-1][1][:16]}")

    if not mismatch:
        print("\ncapture comparison OK: no key has a different value")
        return 0
    if args.strict:
        print("\nstrict: failing on value mismatch")
        return 1
    print("\nvalue mismatches present (exit 0 unless --strict)")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="mode", required=True)

    g = sub.add_parser("golden", help="compare a replay run with the golden file")
    g.add_argument("--replay", required=True)
    g.add_argument("--golden", required=True)
    g.add_argument("--capture", default="", help="only used in the error message")
    g.add_argument("--update", action="store_true")
    g.set_defaults(func=cmd_golden)

    c = sub.add_parser("capture", help="compare a replay run with the P lines of the same capture")
    c.add_argument("--replay", required=True)
    c.add_argument("--capture", required=True)
    c.add_argument("--strict", action="store_true")
    c.set_defaults(func=cmd_capture)

    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
