#!/usr/bin/env bash
# Summarise one PlatformIO build for the GitHub job summary: firmware version,
# flash/RAM usage, binary size and SHA-256 (written next to the binary).
#
# Usage: firmware_summary.sh <pio-env> <build-log> <binary-path>
#
# Part of the P1P2MQTT fork (https://github.com/alxzndr/daikin-p1p2).
# License: CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
#
# Read-only: it never uploads, flashes or contacts a device.
set -uo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <pio-env> <build-log> <binary-path>" >&2
  exit 2
fi

env_name="$1"
log_file="$2"
bin_file="$3"

# ESP8266 prints "RAM: [..] x% (used N bytes from M bytes)"; MiniCore prints
# "Program: N bytes (x% of used Program Space)". Strip any ANSI colour codes first.
size_lines=""
if [ -f "$log_file" ]; then
  # Literal ESC keeps this working with both BSD and GNU sed.
  esc=$(printf '\033')
  size_lines=$(sed -e "s/${esc}\[[0-9;]*[A-Za-z]//g" "$log_file" \
    | grep -E '^(RAM|Flash|Program|Data):' || true)
fi

built="unknown"
if [ -f "$log_file" ]; then
  if grep -qE '\[SUCCESS\]' "$log_file"; then
    built="yes"
  elif grep -qE '\[FAILED\]|error:' "$log_file"; then
    built="no"
  fi
fi

# Version string as HA shows it (sw_version), from the config header of either firmware.
version=$(grep -hE '^#define[[:space:]]+(HA_SW|WELCOMESTRING)[[:space:]]' \
  P1P2_Config.h P1P2Config.h 2>/dev/null | head -1 | sed -E 's/.*"(.*)".*/\1/' || true)
[ -n "$version" ] || version="(version not found)"

sha256="(no binary)"
if [ -f "$bin_file" ]; then
  size=$(wc -c <"$bin_file" | tr -d ' ')
  if command -v sha256sum >/dev/null 2>&1; then
    sha_line=$(sha256sum "$bin_file")
  else
    sha_line=$(shasum -a 256 "$bin_file")
  fi
  printf '%s\n' "$sha_line" >"${bin_file}.sha256"
  sha256=$(printf '%s' "$sha_line" | awk '{print $1}')
else
  size="(missing)"
fi

{
  echo "### Firmware build: \`${env_name}\`"
  echo
  echo "| | |"
  echo "|---|---|"
  echo "| Firmware version | \`$version\` |"
  echo "| Build succeeded | $built |"
  echo "| Binary | \`$bin_file\` ($size bytes) |"
  echo "| SHA-256 | \`$sha256\` |"
  echo
  if [ -n "$size_lines" ]; then
    echo '```'
    printf '%s\n' "$size_lines"
    echo '```'
  else
    echo "_No flash/RAM usage lines in the build log (build did not reach the link step)._"
  fi
} >>"${GITHUB_STEP_SUMMARY:-/dev/null}"

# Also print to the job log so the run is readable without the summary.
echo "== $env_name: $version"
printf '%s\n' "$size_lines"
echo "== $env_name sha256: $sha256"
