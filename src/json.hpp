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

// ---- Señal de salud HTTP (invocada desde http.cpp) ----
// Si ya tienes otra implementación en otro módulo, deja esta tal cual;
// si no, esta versión sirve de no-op y no rompe nada.
void marcaHttpOk(bool ok);

// ============================================================================
// NOTA: Las variables globales usadas por estas funciones (debugSerie, flags,
// buffers, etc.) están declaradas como extern en config.hpp y definidas en tu
// módulo de globals. Este header NO define globals nuevas.
// ============================================================================
