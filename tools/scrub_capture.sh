#!/usr/bin/env bash
# Turn a raw capture from tools/capture.sh into a copy that is safe to commit.
#
# Usage: tools/scrub_capture.sh [-f] [-o DIR] FILE [FILE...]
#
# For every input file it writes <name>_scrubbed.log containing only:
#   * the header comments, with the broker address replaced by <redacted>;
#   * P1P2/R lines (raw packets: the input for the offline decode tests);
#   * P1P2/P lines (the parameter values the bridge published).
# Everything else is dropped and reported:
#   * P1P2/S lines - verbose bridge status, the place where config values such as
#     the OTA or WiFi password can appear;
#   * lines on any other topic (defensive, the capture should not contain them);
#   * lines matching a credential-like pattern (listed below).
#
# Part of the P1P2MQTT fork (https://github.com/alxzndr/daikin-p1p2).
# License: CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
set -euo pipefail

PROGRAM=$(basename "$0")

usage() {
  cat <<EOF
$PROGRAM - make capture file(s) safe to commit

usage: $PROGRAM [-f] [-o DIR] FILE [FILE...]

  FILE     capture file(s) written by tools/capture.sh
  -o DIR   write the scrubbed copies here (default: next to the input file)
  -f       overwrite an existing *_scrubbed.log
  -h       this help

Reads the input, writes only *_scrubbed.log. Never contacts the broker or the bus.
It prints every line it drops, so you can check that nothing important is lost -
and read the result again before you commit.
EOF
}

force=0
out_dir=""

while getopts ":o:fh" opt; do
  case "$opt" in
    o) out_dir=$OPTARG ;;
    f) force=1 ;;
    h) usage; exit 0 ;;
    \?) usage >&2; exit 2 ;;
    :) usage >&2; exit 2 ;;
  esac
done
shift $((OPTIND - 1))

[ "$#" -ge 1 ] || { usage >&2; exit 2; }
if [ -n "$out_dir" ]; then
  mkdir -p "$out_dir"
fi

# Lowercase literal substrings; a line containing one is dropped instead of
# guessed at. Chosen so that no real P1P2 parameter key matches (checked against
# the names in P1P2_HomeAssistant.h / P1P2_ParameterConversion.h): "ota" for
# example would hit every *_Total counter and is therefore not in the list - it
# only ever appears on P1P2/S lines, which are dropped anyway.
PATTERNS=(
  password passwd passphrase pwd psk secret token
  api_key apikey credential private_key bearer auth
  '-P ' --user ssid wlan wifi mqtt_pass
)
# Tab-separated: awk -v cannot carry newlines, and one pattern contains a space.
tab=$(printf '\t')
pattern_str=""
for p in "${PATTERNS[@]}"; do
  pattern_str="${pattern_str:+$pattern_str$tab}$p"
done

fail=0
for in_file in "$@"; do
  if [ ! -f "$in_file" ]; then
    echo "$PROGRAM: $in_file: no such file" >&2
    fail=1
    continue
  fi

  base=$(basename "$in_file")
  case "$base" in
    *_scrubbed.log)
      echo "$PROGRAM: $base: already scrubbed, skipping" >&2
      continue
      ;;
  esac
  dest="${out_dir:-$(dirname "$in_file")}/${base%.log}_scrubbed.log"
  if [ -e "$dest" ] && [ "$force" -ne 1 ]; then
    echo "$PROGRAM: $dest already exists (use -f to overwrite), skipping" >&2
    continue
  fi

  kept_file=$(mktemp "${TMPDIR:-/tmp}/scrub_capture.XXXXXX")
  report_file=$(mktemp "${TMPDIR:-/tmp}/scrub_report.XXXXXX")
  trap 'rm -f "$kept_file" "$report_file"' EXIT

  awk \
    -v patterns="$pattern_str" \
    -v name="$base" \
    -v report="$report_file" '
    BEGIN {
      n = split(patterns, pat, "\t")
      kept = 0; drop_status = 0; drop_topic = 0; drop_secret = 0
    }
    /^#/ {
      if ($0 ~ /^# broker:/) { print "# broker: <redacted>" } else { print }
      next
    }
    {
      topic = $3
      if (topic ~ /^P1P2\/S(\/|$)/) {
        drop_status++
        printf "%s:%d  drop P1P2/S line (topic %s)\n", name, NR, topic >> report
        next
      }
      if (topic !~ /^P1P2\/(R|P)(\/|$)/) {
        drop_topic++
        printf "%s:%d  drop unexpected topic: %s\n", name, NR, topic >> report
        next
      }
      lower = tolower($0)
      for (i = 1; i <= n; i++) {
        if (pat[i] == "") continue
        if (index(lower, pat[i]) > 0) {
          drop_secret++
          printf "%s:%d  drop line matching credential pattern \"%s\" (topic %s)\n", \
            name, NR, pat[i], topic >> report
          next
        }
      }
      kept++
      print
    }
    END {
      printf "kept %d, dropped: P1P2/S %d, other topic %d, credential pattern %d\n", \
        kept, drop_status, drop_topic, drop_secret >> report
    }
    ' "$in_file" >"$kept_file"

  mv "$kept_file" "$dest"
  trap - EXIT

  echo "== $base -> $dest"
  if [ -s "$report_file" ]; then
    sed -e 's/^/   /' <"$report_file"
  else
    echo "   nothing removed"
  fi
  rm -f "$report_file"

  # Self-check: no line on P1P2/S may survive (header comments are not topics).
  if awk 'BEGIN { found = 0 }
          END { exit found }
          $0 !~ /^#/ && $3 ~ /^P1P2\/S(\/|$)/ { found = 1; exit }' "$dest"; then
    :
  else
    echo "$PROGRAM: $dest still has a P1P2/S topic - not calling it clean" >&2
    fail=1
  fi
done

if [ "$fail" -ne 0 ]; then
  exit 1
fi
echo "done. Read the scrubbed file (and # broker: <redacted>) before you commit it."
