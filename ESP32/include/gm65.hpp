#ifndef GM65_HPP
#define GM65_HPP

#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "types.hpp" // QRKind

// ============================================================================
// GM65 — Lector de códigos (UART2). Normaliza tres formatos de QR:
//   1) ODOO: URL con access_token de 36 chars → devuelve el token (36)
//   2) TEC : URL con ticket_id=######&event_id=###### → devuelve "ticket_id=..&event_id=.."
//   3) MAGE: numérico puro de 19 dígitos → devuelve el número
// Cualquier otra cosa se descarta.
//
//Las placas que van del ME001 al ME004 tienen el GM65 configurado en UART2 (RX=26, TX=27)
//Las placas que van del ME005 al ME010 en tienen el GM65 configurado en UART2 (RX=27, TX=26)
// ============================================================================

// Pines UART del GM65 (ajusta si fuese necesario)
#define PIN_GM65_RX 27
#define PIN_GM65_TX 26
#define GM65_BAUD 9600

namespace GM65{
// Inicializa UART2 para el GM65
void begin();

// Lee una línea CR/LF cruda del lector en 'out' (máx ~512 chars). Devuelve true si hay línea.
bool readLine(String &out);

// Lee línea, normaliza y clasifica. Devuelve false si no pasa reglas estrictas.
// Si 'kindOut' no es null, escribe ahí el tipo detectado.
bool readLine_parsed(String &outCode, QRKind *kindOut = nullptr);
}
#endif // GM65_HPP