#pragma once
#include <stdint.h>

// ============================================================================
// TIPOS/ENUMS COMPARTIDOS ENTRE MÓDULOS
// ============================================================================

// Tipo de comando entre tareas (cola IO -> NET y viceversa)
enum CmdType : uint8_t {
  CMD_NONE = 0,
  CMD_READY,
  CMD_OPEN_CONTINUOUS,
  CMD_VALIDATE_IN,
  CMD_VALIDATE_OUT,
  CMD_PASS_OK,
  CMD_PASS_TIMEOUT,
  CMD_PASSED_IN,
  CMD_PASSED_OUT,
  CMD_UPDATE
};

// Clasificación de QR (según prefijo/origen del dato leído)
enum QRKind : uint8_t {
  QR_ODOO    = 0,
  QR_TEC     = 1,
  QR_MAGE    = 2,
  QR_UNKNOWN = 255
};

// Resultado de la validación online (validateQR)
enum ValidateOutcome : uint8_t {
  VNONE = 0,      // sin resultado aún
  VAUTH_IN,       // 201 autorizado entrada
  VAUTH_OUT,      // 202 autorizado salida
  VDENIED,        // r=KO con 4xx/5xx (p.ej., 409 TICKET_ALREADY_USED)
  VERROR          // error de parseo / HTTP sin cuerpo / otras condiciones
};

// Helper textual para logs/debug
static inline const char* qrKindToStr(QRKind k) {
  switch (k) {
    case QR_ODOO:    return "ODOO";
    case QR_TEC:     return "TEC";
    case QR_MAGE:    return "MAGE";
    case QR_UNKNOWN: return "UNKNOWN";
    default:         return "?";
  }
}
