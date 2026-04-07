#ifndef JSON_HPP
#define JSON_HPP
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "types.hpp"

// ============================================================================
// JSON: serializadores / deserializadores y helpers de estado
// ¡Mantiene contratos existentes!
// ============================================================================

// ---- Serializadores (en el orden que usas en el resto del proyecto) ----
void serializaInicio();        // → outputInicio
void serializaEstado();        // → outputEstado
void serializaQR();            // → outputTicket (incluye ultimoTicket)
void serializaPaso();          // → outputPaso   (incluye ultimoPaso)
void serializaReportFailure(); // → outputInicio (reutilizado)

// ---- Deserializadores (llenan estado desde *Recibido) ----
void descifraEstado();
void descifraQR();
void descifraPaso();

void resetCycleReady();
// ============================================================================
// NOTA: Las variables globales usadas por estas funciones (debugSerie, flags,
// buffers, etc.) están declaradas como extern en config.hpp y definidas en tu
// módulo de globals. Este header NO define globals nuevas.
// ============================================================================

#endif // JSON_HPP