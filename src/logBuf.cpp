#include "logBuf.hpp"

#include <stdarg.h>

static LogEntry g_logs[LOGBUF_CAP];
static uint32_t g_seq = 0;
static uint16_t g_head = 0;

// Gate para no generar logs cuando no hay sesión
static volatile bool g_enabled = false;

// Mutex ligero (ISR-safe) para ESP32
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

static String jsonEscape(const char* s)
{
  String out;
  for (const char* p = s; *p; ++p) {
    char c = *p;
    if (c == '\\' || c == '"') { out += '\\'; out += c; }
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

void logbuf_begin()
{
  logbuf_clear();
  g_enabled = false;
}

void logbuf_enable(bool on)
{
  g_enabled = on;
}

bool logbuf_enabled()
{
  return g_enabled;
}

void logbuf_clear()
{
  portENTER_CRITICAL(&g_mux);
  memset(g_logs, 0, sizeof(g_logs));
  g_seq = 0;
  g_head = 0;
  portEXIT_CRITICAL(&g_mux);
}

void logbuf_pushf(const char* fmt, ...)
{
  if (!g_enabled) return;  // CLAVE: coste casi cero si está desactivado

  char line[LOGBUF_LINE];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);

  portENTER_CRITICAL(&g_mux);
  LogEntry &e = g_logs[g_head];
  e.id = ++g_seq;
  e.ms = millis();
  strlcpy(e.msg, line, sizeof(e.msg));
  g_head = (g_head + 1) % LOGBUF_CAP;
  portEXIT_CRITICAL(&g_mux);
}

String logbuf_get_json_since(uint32_t since, uint32_t& outNext)
{
  // ====== SNAPSHOT FUERA DE STACK (CRÍTICO) ======
  static LogEntry snap[LOGBUF_CAP];

  uint16_t head;
  uint32_t seq;

  portENTER_CRITICAL(&g_mux);
  memcpy(snap, g_logs, sizeof(snap));
  head = g_head;
  seq  = g_seq;
  portEXIT_CRITICAL(&g_mux);

  outNext = seq;

  // Limitar items para no generar JSON enorme ni tardar demasiado
  const uint16_t MAX_ITEMS = 40;

  String body;
  body.reserve(2048);  // Ajustado a un JSON razonable con límite
  body += "{\"next\":";
  body += String(seq);
  body += ",\"items\":[";

  bool first = true;
  uint16_t count = 0;

  // Recorremos en orden cronológico
  for (uint16_t i = 0; i < LOGBUF_CAP; ++i)
  {
    const LogEntry &e = snap[(head + i) % LOGBUF_CAP];
    if (e.id == 0) continue;
    if (e.id <= since) continue;

    if (!first) body += ",";
    first = false;

    body += "{\"id\":";
    body += String(e.id);
    body += ",\"ms\":";
    body += String(e.ms);
    body += ",\"msg\":\"";

    // Escape inline (evita jsonEscape() que crea Strings temporales)
    for (const char* p = e.msg; *p; ++p)
    {
      char c = *p;
      if (c == '\\' || c == '"') { body += '\\'; body += c; }
      else if (c == '\n') body += "\\n";
      else if (c == '\r') body += "\\r";
      else if (c == '\t') body += "\\t";
      else body += c;
    }

    body += "\"}";

    if (++count >= MAX_ITEMS) break;
  }

  body += "]}";
  return body;
}
