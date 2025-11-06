// time.cpp — Sincronización de hora (NTP + fallback por cabecera HTTP Date)
#include "time.hpp"
#include "config.hpp"      // serverURL/DEVICE_ID + HTTP_DATE_* (si activado)
#include "http.hpp"        // httpGetRaw(), serializaEstado()
#include <Ethernet.h>
#include <time.h>
#include <sys/time.h>

// ===== Defaults sobreescribibles por config.hpp =====
#ifndef TZ_ESP
  #define TZ_ESP "CET-1CEST,M3.5.0/2,M10.5.0/3"
#endif

#ifndef HORA_MIN_VALIDA
  #define HORA_MIN_VALIDA ((time_t)1700000000)   // ~Nov 2023
#endif

#ifndef RESYNC_PERIOD_MIN
  #define RESYNC_PERIOD_MIN 15
#endif

#ifndef ENABLE_HTTP_DATE_SYNC
  #define ENABLE_HTTP_DATE_SYNC 1
#endif

#ifndef NTP_DNS1
  #define NTP_DNS1 "pool.ntp.org"
#endif
#ifndef NTP_DNS2
  #define NTP_DNS2 "time.google.com"
#endif
#ifndef NTP_DNS3
  #define NTP_DNS3 "time.cloudflare.com"
#endif

#ifndef HTTP_DATE_HOST
  #define HTTP_DATE_HOST "10.0.0.10"
#endif
#ifndef HTTP_DATE_PORT
  #define HTTP_DATE_PORT 8080
#endif
// El path se monta como: "/PTService/ESP32/entries?id=" + DEVICE_ID

// ===== Estado interno =====
static bool          g_ntpIniciado  = false;
static uint32_t      g_nextSyncMs   = 0;
static unsigned long g_baseSegsDia  = 0;
static unsigned long g_baseMillis   = 0;

// -----------------------------------
// Zona horaria
void configurarZonaHoraria() {
  setenv("TZ", TZ_ESP, 1);
  tzset();
}

// -----------------------------------
// NTP
static void iniciarNTP() {
  if (g_ntpIniciado) return;
  configTzTime(TZ_ESP, NTP_DNS1, NTP_DNS2, NTP_DNS3);
  g_ntpIniciado = true;
  Serial.println(F("[NTP] configTzTime()"));
}

static bool esperarNTP(uint32_t timeoutMs) {
  const uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    const time_t now = time(nullptr);
    if (now != (time_t)-1 && now > HORA_MIN_VALIDA) return true;
    delay(200);
  }
  return false;
}

// -----------------------------------
// Helpers tiempo
static void refrescarFotoSegsDia() {
  time_t now = time(nullptr);
  if (now <= HORA_MIN_VALIDA) return;
  struct tm t{};
  if (localtime_r(&now, &t)) {
    g_baseSegsDia = (unsigned long)t.tm_hour * 3600UL
                  + (unsigned long)t.tm_min  * 60UL
                  + (unsigned long)t.tm_sec;
    g_baseMillis  = millis();
  }
}

// Parser de cabecera HTTP "Date: Tue, 04 Nov 2025 12:34:56 GMT"
static bool parseHttpDateToEpochUTC(const String& headers, time_t &utcOut) {
  int i = headers.indexOf("\nDate:");
  if (i < 0) i = headers.indexOf("\r\nDate:");
  if (i < 0) return false;

  int lineStart = headers.indexOf(':', i); if (lineStart < 0) return false;
  lineStart++;
  while (lineStart < (int)headers.length() && headers[lineStart] == ' ') lineStart++;

  int lineEnd = headers.indexOf("\r\n", lineStart);
  if (lineEnd < 0) lineEnd = headers.indexOf('\n', lineStart);
  if (lineEnd < 0) lineEnd = headers.length();

  String dh = headers.substring(lineStart, lineEnd); // "Tue, 04 Nov 2025 12:34:56 GMT"

  struct tm tm{}; int day, year, hour, min, sec; char mon[4] = {0};
  const char* s = dh.c_str();
  const char* comma = strchr(s, ','); if (!comma) return false;
  s = comma + 2;
  if (sscanf(s, "%d %3s %d %d:%d:%d GMT", &day, mon, &year, &hour, &min, &sec) != 6) return false;

  const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char* p = strstr(months, mon); if (!p) return false;
  int monNum = (p - months)/3;

  // Forzar TZ UTC para interpretar como UTC
  char oldtz[64] = {0};
  const char* tz = getenv("TZ");
  if (tz) strncpy(oldtz, tz, sizeof(oldtz)-1);
  setenv("TZ", "UTC0", 1); tzset();

  tm.tm_mday = day; tm.tm_mon = monNum; tm.tm_year = year-1900;
  tm.tm_hour = hour; tm.tm_min = min; tm.tm_sec = sec;
  time_t utc = mktime(&tm);

  // Restaurar TZ original
  if (tz && oldtz[0]) setenv("TZ", oldtz, 1); else unsetenv("TZ");
  tzset();

  utcOut = utc;
  return utcOut > 0;
}

// -----------------------------------
// Fallback: sincronizar leyendo cabecera HTTP Date de GET /entries
static bool syncFromHttpDate() {
#if ENABLE_HTTP_DATE_SYNC
  // 1) Construir PATH y heartbeat mínimo
  String path = String("/PTService/ESP32/entries?id=") + DEVICE_ID;

  // 2) GET crudo para obtener HEADERS con 'Date'
  String headers, body;
  if (!httpGetRaw(HTTP_DATE_HOST, HTTP_DATE_PORT, path.c_str(), headers, body)) {
    if (debugSerie) Serial.println(F("[TIME][HTTP] httpGetRaw() falló"));
    return false;
  }

  // 3) Parsear cabecera Date → epoch UTC
  time_t utc = 0;
  if (!parseHttpDateToEpochUTC(headers, utc)) {
    if (debugSerie) {
      Serial.println(F("[TIME][HTTP] No se pudo parsear cabecera Date"));
      Serial.println(headers);
    }
    return false;
  }

  // 4) Aplicar hora
  struct timeval tv = { .tv_sec = utc, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  configurarZonaHoraria();
  refrescarFotoSegsDia();

  if (debugSerie) {
    struct tm t{};
    time_t now = time(nullptr);
    localtime_r(&now, &t);
    Serial.printf("[TIME] Sincronizado por HTTP Date: %02d:%02d:%02d epoch=%ld\n",
                  t.tm_hour, t.tm_min, t.tm_sec, (long)now);
  }
  return true;
#else
  return false;
#endif
}

// ===== API pública =====
bool syncHoraInicio(uint32_t timeoutMs) {
  // Requiere red activa
  if (Ethernet.linkStatus() != LinkON || Ethernet.localIP() == IPAddress(0,0,0,0)) {
    Serial.println(F("[TIME] Sin red: no puedo sincronizar"));
    return false;
  }

  configurarZonaHoraria();

  // Plan A: NTP
  iniciarNTP();
  bool ok = esperarNTP(timeoutMs);

  // Plan B: HTTP Date si NTP falla
  if (!ok) {
    Serial.println(F("[TIME] NTP KO (timeout) → intentando HTTP Date"));
    ok = syncFromHttpDate();
  }

  if (ok) {
    refrescarFotoSegsDia();
    g_nextSyncMs = millis() + (uint32_t)RESYNC_PERIOD_MIN*60UL*1000UL;

    time_t now = time(nullptr);
    struct tm t{};
    localtime_r(&now, &t);
    Serial.printf("⏱️ Hora OK %02d:%02d:%02d  epoch=%ld  tz=%s\n",
                  t.tm_hour, t.tm_min, t.tm_sec, (long)now, getenv("TZ"));
  } else {
    Serial.println(F("[TIME] Sincronización KO (NTP y HTTP Date fallaron)"));
  }
  return ok;
}

void loop_time_sync(uint32_t periodMin) {
  const uint32_t nowMs = millis();
  if ((int32_t)(nowMs - g_nextSyncMs) < 0) return;

  bool resynced = false;

  // Si ya hay hora válida, solo reprogramamos
  time_t now = time(nullptr);
  if (now > HORA_MIN_VALIDA) {
    refrescarFotoSegsDia();
    g_nextSyncMs = nowMs + periodMin*60UL*1000UL;
    return;
  }

  // Intentar NTP rápido
  if (Ethernet.linkStatus() == LinkON && Ethernet.localIP() != IPAddress(0,0,0,0)) {
    iniciarNTP();
    if (esperarNTP(3000)) {
      refrescarFotoSegsDia();
      g_nextSyncMs = nowMs + periodMin*60UL*1000UL;
      resynced = true;
    }
  }

  // Si NTP falla, intentar HTTP Date
  if (!resynced) {
    if (syncFromHttpDate()) {
      g_nextSyncMs = nowMs + periodMin*60UL*1000UL;
      resynced = true;
    }
  }

  // Si todo falla, reintentar en 30s
  if (!resynced) {
    g_nextSyncMs = nowMs + 30000UL;
  }
}

unsigned long segundosActualesDelDia() {
  time_t now = time(nullptr);
  struct tm t{};
  if (now > HORA_MIN_VALIDA && localtime_r(&now, &t)) {
    return (unsigned long)t.tm_hour*3600UL
         + (unsigned long)t.tm_min *60UL
         + (unsigned long)t.tm_sec;
  }
  // Fallback: “foto” interna
  return g_baseSegsDia + (millis() - g_baseMillis)/1000UL;
}

bool horaLocal_HHMMSS(char out[9]) {
  time_t now = time(nullptr);
  if (now <= HORA_MIN_VALIDA) return false;
  struct tm t{};
  if (!localtime_r(&now, &t)) return false;
  snprintf(out, 9, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  return true;
}

bool horaLocal_ISO(char out[20]) {
  time_t now = time(nullptr);
  if (now <= HORA_MIN_VALIDA) return false;
  struct tm t{};
  if (!localtime_r(&now, &t)) return false;
  snprintf(out, 20, "%04d-%02d-%02d %02d:%02d:%02d",
           t.tm_year+1900, t.tm_mon+1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
  return true;
}

time_t epochActual() {
  return time(nullptr);
}
