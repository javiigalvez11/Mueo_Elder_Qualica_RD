#pragma once
#include <Arduino.h>

// Ajustes
#ifndef LOGBUF_CAP
#define LOGBUF_CAP 200          // nº de líneas en RAM
#endif

#ifndef LOGBUF_LINE
#define LOGBUF_LINE 160         // tamaño max por línea
#endif

struct LogEntry {
  uint32_t id;
  uint32_t ms;
  char msg[LOGBUF_LINE];
};

// Inicializa el buffer (llamar una vez en setup)
void logbuf_begin();

// Habilita/deshabilita el logging (para evitar overhead cuando nadie mira)
void logbuf_enable(bool on);
bool logbuf_enabled();

// Limpia el buffer (RAM)
void logbuf_clear();

// Push “printf-style”
void logbuf_pushf(const char* fmt, ...);

// Devuelve JSON con items nuevos desde `since`.
// `outNext` te devuelve el último id disponible (cursor).
String logbuf_get_json_since(uint32_t since, uint32_t& outNext);
