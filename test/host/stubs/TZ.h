/* TZ.h — host stub for the replay harness (see stubs/Arduino.h).
 * The firmware only needs the TZ_* string macros and configTime(). configTime()
 * sets the process timezone so that localtime_r() in the firmware resolves to
 * the same wall-clock fields the bridge would have shown - the epoch itself is
 * supplied by the capture, never by the host clock.
 * Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
 */
#ifndef HOST_STUB_TZ_H
#define HOST_STUB_TZ_H
#include <ctime>

#define TZ_none ""
#define TZ_user ""
#define TZ_Europe_Amsterdam "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define TZ_Europe_London "GMT0BST,M3.5.0/1,M10.5.0"
#define TZ_Europe_Madrid "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define TZ_Europe_Paris "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define TZ_America_New_York "EST5EDT,M3.2.0/2,M11.1.0/2"
#define TZ_America_Chicago "CST6CDT,M3.2.0/2,M11.1.0/2"
#define TZ_America_Denver "MST7MDT,M3.2.0/2,M11.1.0/2"
#define TZ_America_Los_Angeles "PST8PDT,M3.2.0/2,M11.1.0/2"
#define TZ_Asia_Tokyo "JST-9"
#define TZ_Australia_Sydney "AEST-10AEDT,M10.1.0,M4.1.0/3"

void configTime(const char *tz, const char *ntpServer, long gmtOffsetSec = 0);
inline void configTime(int, int, const char *) {}
#endif
