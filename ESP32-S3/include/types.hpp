#ifndef TYPES_HPP
#define TYPES_HPP

#pragma once
#include <stdint.h>

// ============================================================================
// TIPOS/ENUMS COMPARTIDOS ENTRE MÓDULOS
// ============================================================================
// Estados de la máquina principal (TaskIO)
enum StateIO {
    ST_IDLE,            // Esperando lectura
    ST_VALIDATING,      // Esperando respuesta del servidor
    ST_WAITING_PASS     // Torno desbloqueado, contando pasos
};

// Tipo de comando entre tareas (cola IO -> NET y viceversa)
enum CmdType {
  CMD_NONE = 0,
  CMD_READY,
  CMD_OPEN_CONTINUOUS,
  CMD_ENTRY_CONTINUOUS,
  CMD_OUTPUT_CONTINUOUS,
  CMD_FIN_CONTINUOUS,
  CMD_VALIDATE_IN,
  CMD_VALIDATE_OUT,
  CMD_PASS_OK,
  CMD_PASS_TIMEOUT,
  CMD_PASS_IN,
  CMD_PASS_OUT,
  CMD_BAD_REQUEST,
  CMD_UNAUTHORIZED,
  CMD_ALREADY_USED,
  CMD_RESTART,
  CMD_ABORT,
  CMD_UPDATE,
  CMD_FAIL_REPORT
};

// Resultado de la validación online (validateQR)
enum ValidateOutcome : uint8_t {
  VNONE = 0,      // sin resultado aún
  VAUTH_IN,       // 201 autorizado entrada
  VAUTH_OUT,      // 202 autorizado salida
  VDENIED,        // r=KO con 4xx/5xx (p.ej., 409 TICKET_ALREADY_USED)
  VTIME_NOT_YET,  // entrada valida pero no es su hora actualmente.
  VERROR          // error de parseo / HTTP sin cuerpo / otras condiciones
};

// Clasificación de QR (según prefijo/origen del dato leído)
enum QRKind : uint8_t {
  QR_ODOO    = 0,
  QR_TEC     = 1,
  QR_MAGE    = 2,
  QR_UNKNOWN = 255
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


// =================== Mensajería entre tareas ===================
typedef struct
{
  CmdType type;
  char payload[128];
} CmdMsg;


// Respuesta interna desde TaskNet hacia TaskIO
struct ServerReply {
    bool autorizado;
    int pasosTotales;
    int direccion; // 1: Entrada, 2: Salida
};



#endif // TYPES_HPP
