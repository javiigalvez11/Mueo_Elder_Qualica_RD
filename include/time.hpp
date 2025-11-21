#ifndef TIME_HPP
#define TIME_HPP

#pragma once
#include <Arduino.h>

// ============================================================================
// Sincronización de hora: NTP con fallback a cabecera HTTP Date.
// expone utilidades de hora local y “segundos del día”.
// ============================================================================

// Configurar TZ del sistema (por defecto CET/CEST; configurable vía macro TZ_ESP)
void configurarZonaHoraria();

// Sincroniza hora al inicio: NTP (timeoutMs) → si falla, HTTP Date.
// Requiere red activa (Ethernet link ON e IP no nula).
bool syncHoraInicio(uint32_t timeoutMs);

// Bucle de mantenimiento: re-sincroniza cada 'periodMin' minutos si hace falta.
void loop_time_sync(uint32_t periodMin);

// Segundos actuales dentro del día local (00:00:00 → 0).
unsigned long segundosActualesDelDia();

// "HH:MM:SS" (9 bytes incluyendo terminador). Devuelve false si no hay hora válida.
bool horaLocal_HHMMSS(char out[9]);

// "YYYY-MM-DD HH:MM:SS" (20 bytes incluyendo terminador).
bool horaLocal_ISO(char out[20]);

// Epoch actual (segundos desde 1970, o -1 si sin hora válida).
time_t epochActual();

#endif // TIME_HPP