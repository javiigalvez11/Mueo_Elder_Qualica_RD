#pragma once
#include <Arduino.h>

// ============================================================================
// Cliente HTTP sobre W5500 (solo HTTP claro). Provee:
//  - httpGetRaw, httpPostRaw (cabeceras + body)
//  - API de alto nivel: getInicio, getEstado, postTicket, postPaso, reportFailure
//  - Entradas: getEntradas (texto plano), postPendientesBloque (text/plain)
//  - OTA por HTTP: actualiza()
// ============================================================================

// ===== Parámetros (puedes sobreescribirlos en config.hpp) ====================
#ifndef HTTP_TIMEOUT_MS
  #define HTTP_TIMEOUT_MS   1500    // timeout de socket por operación
#endif
#ifndef HTTP_READ_TIMEOUT_MS
  #define HTTP_READ_TIMEOUT_MS  1500    // timeout acumulado lectura cruda
#endif
#ifndef HTTP_MAX_BYTES
  #define HTTP_MAX_BYTES        32768   // tope de lectura cruda (seguridad)
#endif

// ============ Funciones “raw” (útiles para sincronía de hora por Date) ======
bool httpGetRaw(const char* host, uint16_t port, const char* path,
                String &headersOut, String &bodyOut);

bool httpPostRaw(const char* host, uint16_t port, const char* path,
                 const String& payloadJSON,
                 String& outHeaders, String& outBody);

// ========================= API alto nivel (tu contrato) ======================
void getInicio();              // POST /status  (payload serializaInicio)
void getEstado();              // POST /status  (payload serializaEstado)
void postTicket();             // POST /validateQR
void postPaso();               // POST /validatePass
void reportFailure();          // POST /reportFailure

bool getEntradas(String& outTexto);             // POST /entries (texto plano)
bool postPendientesBloque(const String& txt);   // POST /entries/pending (text/plain)

// =============================== OTA por HTTP ================================
void actualiza();              // Descarga .bin por HTTP y hace Update
