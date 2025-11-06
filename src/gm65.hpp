#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "types.hpp"   // QRKind

// ============================================================================
// GM65 — Lector de códigos (UART2). Normaliza tres formatos de QR:
//   1) ODOO: URL con access_token de 36 chars → devuelve el token (36)
//   2) TEC : URL con ticket_id=######&event_id=###### → devuelve "ticket_id=..&event_id=.."
//   3) MAGE: numérico puro de 19 dígitos → devuelve el número
// Cualquier otra cosa se descarta.
// ============================================================================

// Pines UART del GM65 (ajusta si fuese necesario)
#define PIN_GM65_RX  27
#define PIN_GM65_TX  26
#define GM65_BAUD    9600

// Inicializa UART2 para el GM65
void gm65_begin();

// Lee una línea CR/LF cruda del lector en 'out' (máx ~512 chars). Devuelve true si hay línea.
bool gm65_readLine(String &out);

// Lee línea, normaliza y clasifica. Devuelve false si no pasa reglas estrictas.
// Si 'kindOut' no es null, escribe ahí el tipo detectado.
bool gm65_readLine_parsed(String& outCode, QRKind* kindOut = nullptr);
