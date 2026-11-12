/* replay.cpp — offline replay driver for the P1P2MQTT ESP decoder
 *
 * Part of the P1P2MQTT fork (https://github.com/alxzndr/daikin-p1p2). License:
 * CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 *
 * What this is
 * ------------
 * A host (macOS/Linux) build of the *unmodified* ESP firmware: this file includes
 * examples/P1P2MQTT-bridge/P1P2MQTT-bridge.ino as one translation unit and links it
 * against the stubs in test/host/stubs/. Raw packet lines from a capture are fed in
 * through the stubbed ATmega serial link, so the firmware's own parser, CRC check,
 * decoder, publish-on-change logic and HA discovery generation do all the work. The
 * only replaced transport is AsyncMqttClient::publish(), which records instead of
 * sending (see stubs/AsyncMqttClient.h).
 *
 * Time: millis()/delay()/time() are virtual and advance only with the timestamps in
 * the capture, so a capture always replays to the same output.
 *
 * Usage: see test/host/replay --help  /  docs/ai/TESTING.md
 */

#include FIRMWARE_SKETCH  // set by the Makefile to the .ino of the checkout under test

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <getopt.h>

// ============================================================ virtual clock ====

static uint64_t g_virtualMs = 0;          // milliseconds since the replay started
static uint64_t g_epochBase = 0;          // epoch seconds matching virtual time 0

uint32_t millis() { return (uint32_t)(g_virtualMs & 0xffffffffull); }
uint32_t micros() { return (uint32_t)((g_virtualMs * 1000ull) & 0xffffffffull); }
void delay(uint32_t ms) { g_virtualMs += ms; }
void delayMicroseconds(uint32_t us) { g_virtualMs += us / 1000ull; }
void yield() {}
long random(long howbig) { return howbig > 0 ? 0 : 0; }  // deterministic: no jitter used

static const char *g_posixTz = "CET-1CEST,M3.5.0/02,M10.5.0/03";  // Europe/Amsterdam

void configTime(const char *tz, const char *ntpServer, long gmtOffsetSec) {
  (void)ntpServer;
  (void)gmtOffsetSec;
  if (tz && tz[0]) g_posixTz = tz;  // stored, applied when the capture's epoch is known
}

// The firmware calls time() for its timestamp prefix and for hour/month bookkeeping.
// Overriding the libc symbol in the main executable makes those calls virtual, so a
// replay never depends on the host clock.
extern "C" time_t time(time_t *t) {
  time_t v = (time_t)(g_epochBase + g_virtualMs / 1000ull);
  if (t) *t = v;
  return v;
}

// ============================================================== stub objects ====

const String emptyString("");
const String newlineString("\n");
HostEsp ESP;
HostSerial Serial;
ESP8266WiFiClass WiFi;
MDNSResponder MDNS;
UpdateClass Update;
fs::FS SPIFFS;
ArduinoOTAClass ArduinoOTA;
EepromStub EEPROM;
SPIClassStub SPI;
// mqttClient itself is the firmware object (declared in the sketch); the stub
// class in stubs/AsyncMqttClient.h is what it becomes.

// Referenced by the vendored web update server (see stubs/flash_hal.h).
extern "C" {
char _FS_start = 0;
char _FS_end = 0;
void close_all_fs() {}
// The sketch derives its EEPROM sector from this ESP linker symbol. On a 64-bit host
// the truncated address is meaningless, which is fine: with no --ee-file the harness
// reports "EEPROM read failed" and the firmware falls back to factory defaults, and
// with --ee-file the image is read from offset 0 (see stubs/Arduino.h).
uint32_t _EEPROM_start = 0;
}

char *itoa(int value, char *result, int base) {
  if (base == 16) sprintf(result, "%x", (unsigned)value);
  else if (base == 2) { int i = 0; unsigned v = (unsigned)value; do { result[i++] = char('0' + (v & 1)); v >>= 1; } while (v); result[i] = '\0'; std::reverse(result, result + i); }
  else sprintf(result, "%d", value);
  return result;
}
char *utoa(unsigned value, char *result, int base) { sprintf(result, base == 16 ? "%x" : "%u", value); return result; }
char *ltoa(long value, char *result, int base) { sprintf(result, base == 16 ? "%lx" : "%ld", value); return result; }
char *ultoa(unsigned long value, char *result, int base) { sprintf(result, base == 16 ? "%lx" : "%lu", value); return result; }

#ifndef __APPLE__
size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t n = strlen(src);
  if (size) { size_t c = n < size - 1 ? n : size - 1; memcpy(dst, src, c); dst[c] = '\0'; }
  return n;
}
size_t strlcat(char *dst, const char *src, size_t size) {
  size_t d = strlen(dst), n = strlen(src);
  if (d + n + 1 > size) n = size - d - 1;
  memcpy(dst + d, src, n);
  dst[d + n] = '\0';
  return d + strlen(src);
}
#endif

// ============================================================= serial input ====

static std::string g_serialFeed;
static size_t g_serialPos = 0;

void Serial_hostFeedImpl(const char *data, size_t n) { g_serialFeed.append(data, n); }
void HostSerial::hostFeed(const char *data, size_t n) { Serial_hostFeedImpl(data, n); }
void HostSerial::hostFeedLine(const char *line) { g_serialFeed.append(line); g_serialFeed.push_back('\n'); }
void HostSerial::hostClearFeed() { g_serialFeed.clear(); g_serialPos = 0; }
size_t HostSerial::hostFeedPending() { return g_serialFeed.size() - g_serialPos; }
int HostSerial::available() { return (int)(g_serialFeed.size() - g_serialPos); }
int HostSerial::peek() { return g_serialPos < g_serialFeed.size() ? (unsigned char)g_serialFeed[g_serialPos] : -1; }
int HostSerial::read() { return g_serialPos < g_serialFeed.size() ? (unsigned char)g_serialFeed[g_serialPos++] : -1; }

// =========================================================== publish recorder ==

struct Published {
  uint64_t vms;         // virtual time
  std::string topic;
  uint8_t qos;
  bool retain;
  std::string payload;
};

static std::vector<Published> g_published;
static bool g_linkUp = true;
static uint64_t g_publishCount = 0;

bool hostMqttLinkUp() { return g_linkUp; }

void hostRecordPublish(const char *topic, uint8_t qos, bool retain, const char *payload) {
  Published p;
  p.vms = g_virtualMs;
  p.topic = topic ? topic : "";
  p.qos = qos;
  p.retain = retain;
  p.payload = payload ? payload : "";
  g_published.push_back(p);
  g_publishCount++;
}

// ================================================================== options ====

struct Options {
  std::string capture;
  std::string eeFile;                 // optional EEPROM image (else factory defaults)
  std::string haPrefix;               // override EE.haConfigPrefix when set
  long outputMode = -1;               // -1 = firmware default (INIT_OUTPUTMODE)
  long outputFilter = -1;             // -1 = firmware default (INIT_OUTPUTFILTER)
  uint64_t startEpoch = 0;            // epoch seconds of virtual time 0 (from capture)
  bool haveStartEpoch = false;
  bool includeRaw = false;            // also print the R (raw hex) republish messages
  bool includeStatus = false;         // also print the S (status) messages
  bool feedRetained = false;          // feed retained R lines as events too
  bool noWifi = false;                // pretend WiFi is down (ethernet still up)
  bool noEthernet = false;            // pretend the W5500 is absent (WiFiManager path)
  bool steadyState = true;            // skip the startup throttle ramp
  bool runSetup = true;               // run setup() (default) instead of a hand-picked init
  bool keepPseudoPublishes = true;    // keep values that came from ESP/ATmega pseudo packets
  int maxLoopPerLine = 8;
};

static Options opt;

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s <capture> [options]\n"
          "\n"
          "Reads a capture in the format of tools/capture.sh (see captures/README.md):\n"
          "  <YYYY-MM-DDTHH:MM:SS> <retained 0|1> <topic> <payload>\n"
          "Feeds the P1P2/R lines to the firmware decoder and prints what it publishes.\n"
          "\n"
          "output:  <virtual-seconds> <channel> <retain> <topic> <payload>\n"
          "         channel P = parameter, D = HA discovery, S = status, R = raw hex\n"
          "\n"
          "options:\n"
          "  --include-raw           also print the republished raw hex messages\n"
          "  --include-status         also print the bridge status messages\n"
          "  --feed-retained          treat retained R lines as events (default: skip)\n"
          "  --output-mode <hex>      override EE.outputMode (e.g. 0x13003)\n"
          "  --output-filter <n>      override EE.outputFilter\n"
          "  --ee-file <path>         load EE from this 4 kB flash-sector image\n"
          "  --ha-prefix <prefix>     override EE.haConfigPrefix\n"
          "  --no-throttle-skip       keep the firmware startup throttle ramp (slow)\n"
          "  --minimal-init           do not run setup(); init only what decoding needs\n"
          "  --no-ethernet            pretend the W5500 is absent (WiFiManager path)\n"
          "  --no-wifi                pretend WiFi is disconnected\n"
          "  --quiet                  only print a one line summary to stderr\n",
          argv0);
}

// ============================================================ capture parsing ==

struct CaptureLine {
  std::string ts;      // "YYYY-MM-DDTHH:MM:SS"
  uint64_t epoch = 0;
  bool retained = false;
  std::string topic;
  std::string payload;
};

static bool parseCaptureLine(const std::string &line, CaptureLine &out, std::string &err) {
  // ts, retained, topic, payload(payload may contain spaces)
  size_t p1 = line.find(' ');
  if (p1 == std::string::npos) { err = "no timestamp field"; return false; }
  size_t p2 = line.find(' ', p1 + 1);
  if (p2 == std::string::npos) { err = "no retained flag"; return false; }
  size_t p3 = line.find(' ', p2 + 1);
  if (p3 == std::string::npos) { err = "no topic field"; return false; }
  out.ts = line.substr(0, p1);
  std::string flag = line.substr(p1 + 1, p2 - p1 - 1);
  if (flag != "0" && flag != "1") { err = "retained flag is not 0/1"; return false; }
  out.retained = flag == "1";
  out.topic = line.substr(p2 + 1, p3 - p2 - 1);
  out.payload = line.substr(p3 + 1);
  int y, mo, d, h, mi, s;
  if (sscanf(out.ts.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) != 6) { err = "bad timestamp"; return false; }
  // days since epoch, civil-from-days algorithm (Howard Hinnant), no libc calendar
  int64_t yr = y - (mo <= 2);
  int64_t era = (yr >= 0 ? yr : yr - 399) / 400;
  int64_t yoe = yr - era * 400;
  unsigned mp = (unsigned)(mo + (mo > 2 ? -3 : 9));
  int64_t doy = (153 * mp + 2) / 5 + d - 1;
  int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = era * 146097 + doe - 719468;
  out.epoch = (uint64_t)(days * 86400 + h * 3600 + mi * 60 + s);
  return true;
}

static bool startsWith(const std::string &s, const char *p) { return s.compare(0, strlen(p), p) == 0; }

// The R topic payload is what the ESP published: 'R', then the 20 characters the ESP
// inserted from sprint_value, then the rest of the ATmega line (" T  0.105: <hex>" or
// " P         <hex>"). Feeding that back unchanged would insert the timestamp twice,
// so rebuild the ATmega line by dropping those 20 characters.
static bool reconstructAtmegaLine(const std::string &payload, std::string &out) {
  if (payload.size() < 24) return false;
  const char kind = payload[0];
  if (kind != 'R' && kind != 'E' && kind != 'D' && kind != 'C' && kind != 'c') return false;
  // After the kind letter the ESP inserted exactly 20 characters of its own timestamp
  // (" 2026-11-14 20:31:12" or " ESPuptime      1234"), then the rest of the ATmega
  // line: " T  0.105: <hex>" for bus packets, " P         <hex>" for pseudo packets.
  if (payload[21] == ' ' && payload[22] != ' ' && isalpha((unsigned char)payload[22])) {
    out = std::string(1, kind) + payload.substr(21);
    return true;
  }
  // Already an ATmega line (hand written fixture, e.g. "R T  0.105: 000010...").
  if (payload[1] == ' ' && (payload[2] == 'T' || payload[2] == 'P')) {
    out = payload;
    return true;
  }
  return false;
}

// =============================================================== replay =======

static void applyOptions() {
  if (!opt.eeFile.empty()) ESP.setFlashFile(opt.eeFile.c_str());
#ifdef ETHERNET
  if (opt.noEthernet) eth.hwAvailable = false;  // eth is the firmware's W5500 object
#endif
  WiFi.connected = !opt.noWifi;
}

static void printHelpBanner() {}

// The value of a P topic's key = last path element.
static std::string keyOf(const std::string &topic) {
  size_t p = topic.rfind('/');
  return p == std::string::npos ? topic : topic.substr(p + 1);
}

// Which channel a published message belongs to.
static char channelOf(const Published &p, const std::string &haPrefix) {
  if (startsWith(p.topic, "homeassistant/") ||
      (!haPrefix.empty() && startsWith(p.topic, haPrefix.c_str())))
    return 'D';
  if (startsWith(p.topic, "P1P2/P/")) return 'P';
  if (startsWith(p.topic, "P1P2/R/")) return 'R';
  if (startsWith(p.topic, "P1P2/S/")) return 'S';
  return 'O';  // availability, IP report, ...
}

int main(int argc, char **argv) {
  static struct option longopts[] = {
      {"include-raw", no_argument, 0, 1},
      {"include-status", no_argument, 0, 2},
      {"feed-retained", no_argument, 0, 3},
      {"output-mode", required_argument, 0, 4},
      {"output-filter", required_argument, 0, 5},
      {"ee-file", required_argument, 0, 6},
      {"ha-prefix", required_argument, 0, 7},
      {"no-throttle-skip", no_argument, 0, 8},
      {"minimal-init", no_argument, 0, 9},
      {"no-ethernet", no_argument, 0, 10},
      {"no-wifi", no_argument, 0, 11},
      {"quiet", no_argument, 0, 12},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0},
  };
  bool quiet = false;
  int c;
  while ((c = getopt_long(argc, argv, "h", longopts, nullptr)) != -1) {
    switch (c) {
      case 1: opt.includeRaw = true; break;
      case 2: opt.includeStatus = true; break;
      case 3: opt.feedRetained = true; break;
      case 4: opt.outputMode = strtol(optarg, nullptr, 0); break;
      case 5: opt.outputFilter = strtol(optarg, nullptr, 0); break;
      case 6: opt.eeFile = optarg; break;
      case 7: opt.haPrefix = optarg; break;
      case 8: opt.steadyState = false; break;
      case 9: opt.runSetup = false; break;
      case 10: opt.noEthernet = true; break;
      case 11: opt.noWifi = true; break;
      case 12: quiet = true; break;
      case 'h': usage(argv[0]); return 0;
      default: usage(argv[0]); return 2;
    }
  }
  if (optind >= argc) { usage(argv[0]); return 2; }
  opt.capture = argv[optind];

  FILE *f = fopen(opt.capture.c_str(), "rb");
  if (!f) { fprintf(stderr, "error: cannot read capture %s\n", opt.capture.c_str()); return 2; }

  std::vector<CaptureLine> lines;
  {
    char buf[4096];
    std::string err;
    size_t lineno = 0, skipped = 0;
    while (fgets(buf, sizeof(buf), f)) {
      lineno++;
      std::string s(buf);
      while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
      if (s.empty() || s[0] == '#') continue;
      CaptureLine cl;
      if (!parseCaptureLine(s, cl, err)) {
        fprintf(stderr, "warning: %s:%zu: %s: %.*s\n", opt.capture.c_str(), lineno, err.c_str(), 60, s.c_str());
        skipped++;
        continue;
      }
      lines.push_back(cl);
    }
    fclose(f);
    if (skipped) fprintf(stderr, "warning: %zu unparsable line(s) skipped\n", skipped);
  }
  if (lines.empty()) { fprintf(stderr, "error: no usable lines in %s\n", opt.capture.c_str()); return 2; }

  // The first line's timestamp anchors virtual time; the firmware's clock reads the
  // same wall-clock second as the bridge did.
  for (auto &cl : lines) {
    if (!opt.haveStartEpoch || cl.epoch < opt.startEpoch) { opt.startEpoch = cl.epoch; opt.haveStartEpoch = true; }
  }
  setenv("TZ", g_posixTz, 1);
  tzset();
  // The capture holds local wall-clock stamps; convert to the epoch the bridge's
  // clock would have had (two passes: the offset itself depends on DST).
  g_epochBase = opt.startEpoch;
  for (int pass = 0; pass < 2; pass++) {
    time_t probe = (time_t)g_epochBase;
    struct tm probeTm;
    localtime_r(&probe, &probeTm);
    g_epochBase = (uint64_t)((int64_t)opt.startEpoch - probeTm.tm_gmtoff);
  }

  // ---------------------------------------------------------- start the bridge
  g_linkUp = true;
  applyOptions();
  if (opt.runSetup) {
    setup();
  } else {
    loadEEPROM();
    checkParam();
    loadData();
    buildMqttTopic();
    mqttConnected = 2;
    reportOnlineRestartData();
  }
  if (opt.outputMode >= 0) EE.outputMode = (uint32_t)opt.outputMode;
  if (opt.outputFilter >= 0) EE.outputFilter = (uint8_t)opt.outputFilter;
  if (!opt.haPrefix.empty()) {
    strncpy(EE.haConfigPrefix, opt.haPrefix.c_str(), sizeof(EE.haConfigPrefix) - 1);
    EE.haConfigPrefix[sizeof(EE.haConfigPrefix) - 1] = '\0';
  }
  // Everything published before the first packet is the bridge's birth message
  // (availability, HA discovery, the whole last-known parameter set). A mid-run
  // capture does not contain it, but a replay should still show it: it is exactly
  // what task 03 diffs between firmware revisions.
  size_t startupCount = g_published.size();
  // setup() consumed virtual time (delays in the boot sequence). Capture timestamps
  // are relative to that: the clock must never run backwards.
  const uint64_t vmsAfterBoot = g_virtualMs;

  if (opt.steadyState) {
    // A bridge mid-capture has finished its startup ramp (THROTTLE_VALUE 100 -> 0 in
    // THROTTLE_STEP_S steps). Without this, the first ~100 s of a replay would skip
    // most packets - legitimate, but it makes a comparison unreadable.
    throttleValue = 0;
    throttle = 1;
  }

  // ------------------------------------------------------------- feed the bus
  // After it (re)connects, the ESP ignores the first line from the ATmega. Feed a "*"
  // line to absorb that, so no packet of the capture is swallowed.
  Serial.hostFeedLine("*");
  loop();
  Serial.hostClearFeed();

  size_t fed = 0, skippedRetained = 0, notBus = 0, tooShort = 0;
  for (auto &cl : lines) {
    if (!startsWith(cl.topic, "P1P2/R/")) { notBus++; continue; }
    if (cl.retained && !opt.feedRetained) { skippedRetained++; continue; }
    std::string atmega;
    if (!reconstructAtmegaLine(cl.payload, atmega)) { tooShort++; continue; }
    uint64_t want = vmsAfterBoot + (cl.epoch - opt.startEpoch) * 1000ull;
    if (want > g_virtualMs) g_virtualMs = want;  // stamps in a capture never go back
    Serial.hostFeedLine(atmega.c_str());
    int guard = 0;
    while (Serial.hostFeedPending() && guard++ < opt.maxLoopPerLine) loop();
    if (Serial.hostFeedPending()) {
      fprintf(stderr, "warning: line not consumed after %d loop() calls, flushing\n", guard);
      Serial.hostClearFeed();
    }
    fed++;
  }
  // let the bridge finish what it queued (pseudo packets of the last second)
  g_virtualMs += 1500;
  loop();

  // ------------------------------------------------------------------- output
  std::string haPrefix = EE.haConfigPrefix;
  if (!quiet) {
    printf("# replay of %s\n", opt.capture.c_str());
    printf("# firmware %s (host build, -D%s), TZ %s\n", WELCOMESTRING, SERIES_NAME, g_posixTz);
    printf("# EE.outputMode 0x%04X EE.outputFilter %d\n", (unsigned)EE.outputMode, EE.outputFilter);
    printf("# lines=%zu fed=%zu retained_r_skipped=%zu non_r=%zu malformed_r=%zu published=%zu (of which %zu at startup)\n",
           lines.size(), fed, skippedRetained, notBus, tooShort, g_published.size(), startupCount);
    printf("# columns: <virtual seconds> <channel P|D|R|S|O> <retain> <topic> <payload>\n");
    for (size_t i = 0; i < g_published.size(); i++) {
      const Published &p = g_published[i];
      char ch = channelOf(p, haPrefix);
      if (ch == 'R' && !opt.includeRaw) continue;
      if ((ch == 'S' || ch == 'O') && !opt.includeStatus) continue;
      printf("%llu %c %u %s %s\n", (unsigned long long)((p.vms) / 1000ull), ch, p.retain ? 1u : 0u, p.topic.c_str(),
             p.payload.c_str());
      if (i + 1 == startupCount) printf("# --- startup publishes end here, bus traffic starts ---\n");
    }
  }
  fprintf(stderr, "replay: %zu R lines fed, %zu messages published (%zu at startup)\n", fed, g_published.size(),
          startupCount);
  return 0;
}
