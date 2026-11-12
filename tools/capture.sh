#!/usr/bin/env bash
# Record raw P1/P2 bus traffic from the MQTT broker into captures/ for the offline
# decode tests (see captures/README.md and docs/ai/TESTING.md).
#
# Usage: tools/capture.sh [-d SECONDS] [-o DIR] LABEL
#   LABEL   what you are capturing, e.g. defrost, dhw_run, quiet_on, night_idle
#
# Part of the P1P2MQTT fork (https://github.com/alxzndr/daikin-p1p2).
# License: CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
#
# SAFETY: this script is subscribe-only. It contains no mosquitto_pub, no publish,
# and never writes to P1P2/W*. It must never be pointed at anything but the broker
# that the bridge publishes to; reading is safe, writing is not (docs/ai/SAFETY.md).
set -euo pipefail

PROGRAM=$(basename "$0")
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)

# Broker settings: exported environment variables win, tools/capture.env fills the
# gaps. The env file is git-ignored; copy tools/capture.env.example to create it.
ENV_FILE="${P1P2_CAPTURE_ENV:-$SCRIPT_DIR/capture.env}"

# The three read-only topic trees. P1P2/W* is deliberately absent.
TOPICS=(
  'P1P2/R/#'
  'P1P2/P/#'
  'P1P2/S/#'
)

usage() {
  cat <<EOF
$PROGRAM - subscribe-only capture of the P1/P2 bridge traffic

usage: $PROGRAM [-d SECONDS] [-o DIR] [-e FILE] [-h] LABEL

  LABEL      short name for the situation, becomes part of the file name
             (defrost, dhw_run, quiet_on, night_idle, boost_lwt38, startup, ...)
  -d SECONDS stop automatically after this many seconds (default: until Ctrl-C)
  -o DIR     output directory (default: <repo>/captures)
  -e FILE    environment file with the broker settings
             (default: tools/capture.env, git-ignored)
  -h         this help

Broker settings are read from the environment first, then from the env file:
  P1P2_MQTT_HOST   required, e.g. 10.0.1.5
  P1P2_MQTT_PORT   optional, default 1883
  P1P2_MQTT_USER   optional
  P1P2_MQTT_PASS   optional; mosquitto_sub gets it on its command line, so it is in
                   'ps' while the capture runs - use a read-only broker account
  P1P2_MQTT_CLIENT_ID optional, default p1p2-capture-<pid>

output: <DIR>/YYYY-MM-DD_HHMMSS_LABEL.log, one line per message:
  <ISO timestamp> <topic> <payload>

Nothing in this script can write to the bus or the bridge.
EOF
}

die() {
  echo "$PROGRAM: $*" >&2
  exit 1
}

duration=""
out_dir="$REPO_ROOT/captures"
label=""

while getopts ":d:o:e:h" opt; do
  case "$opt" in
    d) duration=$OPTARG ;;
    o) out_dir=$OPTARG ;;
    e) ENV_FILE=$OPTARG ;;
    h) usage; exit 0 ;;
    \?) usage >&2; die "unknown option -$OPTARG" ;;
    :) usage >&2; die "option -$OPTARG needs a value" ;;
  esac
done
shift $((OPTIND - 1))

[ "$#" -eq 1 ] || { usage >&2; exit 2; }
label=$1

case "$label" in
  *[!A-Za-z0-9._-]*) die "LABEL may only contain letters, digits, dot, dash, underscore (got: $label)" ;;
esac
if [ -n "$duration" ]; then
  case "$duration" in
    *[!0-9]*) die "-d needs a whole number of seconds (got: $duration)" ;;
  esac
  [ "$duration" -ge 5 ] || die "-d is at least 5 seconds"
fi

# ---------------------------------------------------------------- broker config
if [ -f "$ENV_FILE" ]; then
  # Sourced in a subshell so the file cannot change anything but P1P2_MQTT_* values,
  # and never overrides what the caller already exported.
  while IFS= read -r line; do
    key=${line%%=*}
    case "$key" in
      P1P2_MQTT_[A-Z0-9_]*) ;;
      *) continue ;;
    esac
    [ -n "$(printenv "$key" || true)" ] && continue
    export "$key=${line#*=}"
  done < <(
    set -a
    # shellcheck disable=SC1090
    . "$ENV_FILE"
    env | grep '^P1P2_MQTT_' || true
  )
fi

host=${P1P2_MQTT_HOST:-}
port=${P1P2_MQTT_PORT:-1883}
user=${P1P2_MQTT_USER:-}
pass=${P1P2_MQTT_PASS:-}
client_id=${P1P2_MQTT_CLIENT_ID:-p1p2-capture-$$}

[ -n "$host" ] || die "no broker host: set P1P2_MQTT_HOST or put it in $ENV_FILE (see tools/capture.env.example)"
command -v mosquitto_sub >/dev/null 2>&1 || die "mosquitto_sub not found (brew install mosquitto)"

mkdir -p "$out_dir"
out_file="$out_dir/$(date +%Y-%m-%d_%H%M%S)_${label}.log"

args=(
  -h "$host"
  -p "$port"
  -I "$client_id"
  -F '%t %p'
)
# -i/-I only, no -c: a transient subscriber must not become a persistent session
# on the broker that HA also uses.
if [ -n "$user" ]; then
  args+=(-u "$user")
fi
if [ -n "$pass" ]; then
  args+=(-P "$pass")
fi
for topic in "${TOPICS[@]}"; do
  args+=(-t "$topic")
done

# ------------------------------------------------------------------ the capture
{
  echo "# captured with tools/capture.sh on $(date -u +%Y-%m-%dT%H:%M:%S)Z"
  echo "# broker: $host:$port (no credentials are ever written to this file)"
  echo "# topics: ${TOPICS[*]}"
  echo "# label: $label"
} >"$out_file"

echo "subscribing to ${TOPICS[*]} on $host:$port -> $out_file"
echo "stop with Ctrl-C${duration:+, or wait ${duration}s}"

# mosquitto_sub 2.x cannot print a timestamp itself (%Y is not a valid -F specifier),
# so its output goes through a FIFO into a loop that prefixes local time. A FIFO
# (instead of a pipe) keeps the pid of mosquitto_sub, so the timer and Ctrl-C can
# stop it even while the bus is silent.
work_dir=$(mktemp -d -t p1p2-capture)
fifo="$work_dir/pipe"
err_file="$work_dir/stderr"
mkfifo "$fifo"

mosquitto_sub "${args[@]}" >"$fifo" 2>"$err_file" &
sub_pid=$!

stopped_by_us=0
stop_sub() {
  stopped_by_us=1
  kill -TERM "$sub_pid" 2>/dev/null || true
}
trap 'stop_sub' INT TERM
trap 'rm -rf "$work_dir"' EXIT

if [ -n "$duration" ]; then
  # Own timer instead of mosquitto_sub -W: -W only counts while messages arrive,
  # so a quiet bus would keep the capture open far longer than asked.
  (
    sleep "$duration"
    kill -TERM "$sub_pid" 2>/dev/null || true
  ) &
  timer_pid=$!
fi

while IFS= read -r line; do
  printf '%s %s\n' "$(date +%Y-%m-%dT%H:%M:%S)" "$line" >>"$out_file"
done <"$fifo"

set +e
wait "$sub_pid"
rc=$?
if [ -n "${timer_pid:-}" ]; then
  kill -TERM "$timer_pid" 2>/dev/null || true
fi
set -e
stop_sub

# 143 = 128 + SIGTERM: the normal end of a timed or Ctrl-C stopped capture.
if [ "$rc" -ne 0 ] && [ "$rc" -ne 143 ] && [ "$stopped_by_us" -ne 1 ]; then
  if [ -s "$err_file" ]; then
    cat "$err_file" >&2
  fi
  die "mosquitto_sub exited with status $rc (see the message above)"
fi
if [ -s "$err_file" ]; then
  echo "$PROGRAM: messages from mosquitto_sub:" >&2
  cat "$err_file" >&2
fi

lines=$(grep -cv '^#' "$out_file" || true)
echo "wrote $lines message line(s) to $out_file"
if [ "$lines" -eq 0 ]; then
  echo "nothing captured: is the bridge online (P1P2/L/P1P2MQTT/bridge0 = online) and the broker reachable?" >&2
  exit 1
fi
echo "before committing anything from this, scrub it: tools/scrub_capture.sh $out_file"
