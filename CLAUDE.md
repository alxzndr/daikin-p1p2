@AGENTS.md

## Claude Code specifics

- The house context (Home Assistant on 10.0.1.5, the bridge on 192.168.2.110, EMHASS/Daikin control
  automations) is documented in `docs/ai/HOUSE_CONTEXT.md`. The HA MCP tools may be available — use them
  **read-only** for this project (read entity states, MQTT-derived sensors, history) unless the human
  asks for a specific write.
- Build with PlatformIO: `pio` is installed via Homebrew. Builds pull the pinned ESP8266 core from
  `OSS-dependencies/` — do not "upgrade" the platform to fix a build error without asking.
- Do not run `pio run -t upload`, `espota`, telnet commands, or `mosquitto_pub` to `P1P2/W*`. Prepare
  them; the human executes.
- The bridge has **no telnet lock and an unauthenticated web OTA page** (see `docs/ai/HOUSE_CONTEXT.md`).
  Never probe it beyond reading the telnet banner.
