#!/usr/bin/env bash
# run_replay.sh — replay a capture through the ESP decoder built for the host, and
# compare the result with a golden file (default) or with the P messages of the same
# capture (--against-capture). Exits non-zero when something changed.
#
# Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
#
#   test/run_replay.sh captures/2026-11-14_203112_night_idle_scrubbed.log
#   test/run_replay.sh test/fixtures/daikin_e_doc_packets.log --update-golden
#   test/run_replay.sh <capture> --against-capture
#   FIRMWARE_DIR=/tmp/rc23/examples/P1P2MQTT-bridge test/run_replay.sh <capture> --update-golden
#   test/run_replay.sh <capture> --replay-bin /tmp/other-build/replay    # prebuilt binary
#
# The harness is built with test/host/Makefile; pass --rebuild to force it.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
HOST_DIR="$SCRIPT_DIR/host"

usage() {
  sed -n '2,20p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

CAPTURE=""
GOLDEN=""
REPLAY_BIN=""
SERIES="${SERIES:-E_SERIES}"
UPDATE=0
AGAINST=0
REBUILD=0
STRICT=0
KEEP=""
REPLAY_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --update-golden) UPDATE=1; shift ;;
    --against-capture|-c) AGAINST=1; shift ;;
    --golden) GOLDEN="$2"; shift 2 ;;
    --replay-bin) REPLAY_BIN="$2"; shift 2 ;;
    --series) SERIES="$2"; shift 2 ;;
    --rebuild) REBUILD=1; shift ;;
    --strict) STRICT=1; shift ;;
    --keep-output) KEEP="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    --) shift; while [ $# -gt 0 ]; do REPLAY_ARGS+=("$1"); shift; done ;;
    -*) echo "unknown option: $1" >&2; usage; exit 2 ;;
    *) if [ -z "$CAPTURE" ]; then CAPTURE="$1"; shift; else REPLAY_ARGS+=("$1"); shift; fi ;;
  esac
done

[ -n "$CAPTURE" ] || { usage; exit 2; }
[ -f "$CAPTURE" ] || { echo "error: capture not found: $CAPTURE" >&2; exit 2; }

# ---- build the host harness (once per firmware dir + series) -------------------
BUILD_DIR="$HOST_DIR/build/$SERIES"
if [ -n "$REPLAY_BIN" ]; then
  [ -x "$REPLAY_BIN" ] || { echo "error: --replay-bin is not executable: $REPLAY_BIN" >&2; exit 2; }
  REPLAY="$REPLAY_BIN"
else
  if [ "$REBUILD" = 1 ] || [ ! -x "$BUILD_DIR/replay" ]; then
    echo "== building the host harness ($SERIES)"
    make -C "$HOST_DIR" SERIES="$SERIES" ${FIRMWARE_DIR:+FIRMWARE_DIR="$FIRMWARE_DIR"} >/dev/null
  fi
  REPLAY="$BUILD_DIR/replay"
fi

# ---- where the output and the golden go ---------------------------------------
base=$(basename "$CAPTURE"); base="${base%.log}"; base="${base%.log.gz}"
OUT="${KEEP:-$(mktemp "${TMPDIR:-/tmp}/replay_${base}_XXXXXX").txt}"
trap '[ -n "${KEEP:-}" ] || rm -f "$OUT"' EXIT

if [ -z "$GOLDEN" ] && [ "$AGAINST" = 0 ]; then
  case "$CAPTURE" in
    "$REPO_ROOT"/test/fixtures/*) GOLDEN="$REPO_ROOT/test/golden/$base.$SERIES.golden.txt" ;;
    *) GOLDEN="$REPO_ROOT/test/golden/$base.$SERIES.golden.txt" ;;
  esac
fi
mkdir -p "$(dirname "$GOLDEN")" 2>/dev/null || true

echo "== replaying $CAPTURE"
set +e
"$REPLAY" "$CAPTURE" ${REPLAY_ARGS[@]+"${REPLAY_ARGS[@]}"} >"$OUT"
rc=$?
set -e
if [ $rc -ne 0 ]; then
  echo "error: replay failed (exit $rc); output above/above file: $OUT" >&2
  exit $rc
fi
grep '^# ' "$OUT" | sed 's/^# /   /'

FAIL=0

# ---- hand-checked expectations (fixtures only) --------------------------------
EXPECT=""
for candidate in "${CAPTURE%.log}.$SERIES.expect" "${CAPTURE%.log}.expect" \
                 "${CAPTURE%.log.gz}.$SERIES.expect" "${CAPTURE%.log.gz}.expect"; do
  [ -f "$candidate" ] && EXPECT="$candidate" && break
done
if [ -n "$EXPECT" ]; then
  echo "== checking hand-checked expectations: $EXPECT"
  while IFS= read -r line; do
    case "$line" in
      ''|\#*) continue ;;
    esac
    # <channel> <topic> <value>, optionally prefixed with "count:N" to pin how often
    # the message must appear (used to prove publish-on-change behaviour).
    want_n=1
    mode=exact
    case "$line" in
      count:*) want_n=${line#count:}; want_n=${want_n%% *}; line=${line#* } ;;
      contains:*) mode=contains; line=${line#contains: } ;;
    esac
    ch=$(printf '%s' "$line" | awk '{print $1}')
    topic=$(printf '%s' "$line" | awk '{print $2}')
    value=$(printf '%s' "$line" | cut -d' ' -f3-)
    if [ "$mode" = contains ]; then
      got_n=$(grep " $ch [01] $topic " "$OUT" | grep -cF -- "$value" || true)
      [ "$got_n" -gt 0 ] && want_n=$got_n
    else
      got_n=$(grep -c " $ch [01] $topic $value\$" "$OUT" || true)
    fi
    if [ "$got_n" -eq "$want_n" ]; then
      printf '   ok      %-46s %s (%s x)\n' "$topic" "$value" "$want_n"
    else
      printf '   WRONG   %-46s expected %s %s x, got %s x\n' "$topic" "$value" "$want_n" "$got_n"
      FAIL=1
    fi
  done <"$EXPECT"
fi

# ---- comparison mode ----------------------------------------------------------
if [ "$AGAINST" = 1 ]; then
  echo "== comparing with the P messages of the same capture"
  python3 "$HOST_DIR/compare.py" capture --replay "$OUT" --capture "$CAPTURE" ${STRICT:+--strict} || FAIL=1
else
  echo "== comparing with golden $GOLDEN"
  if [ "$UPDATE" = 1 ]; then
    python3 "$HOST_DIR/compare.py" golden --replay "$OUT" --golden "$GOLDEN" --update
  else
    python3 "$HOST_DIR/compare.py" golden --replay "$OUT" --golden "$GOLDEN" --capture "$CAPTURE" || FAIL=1
  fi
fi

if [ "$FAIL" = 0 ]; then
  echo "== PASS"
else
  echo "== FAIL (see the diff / MISSING lines above)"
fi
exit "$FAIL"
