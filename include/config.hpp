#pragma once
#include <Arduino.h>
#include "types.hpp"

// ============================================================================
// CONFIGURACIÓN GLOBAL DEL DISPOSITIVO (ME001 @ Museo Elder)
// ----------------------------------------------------------------------------
// Nota: mantenemos nombres/externs para no romper integración. Este header
// solo declara constantes y 'extern' de estado compartido entre módulos.
// ============================================================================

// =================== Identidad / constantes ===================
static const char* COMERCIO  = "MUSEO_ELDER";
static const char* DEVICE_ID = "ME002";

// =================== Red (Ethernet W5500) =====================
// 1 = IP fija (recomendado en intranet), 0 = DHCP
#define USE_STATIC_IP 1

#if USE_STATIC_IP
  // Ajusta a tu segmento. Si Mikrotik sirve NTP/DNS internos, ponlos aquí.
  static IPAddress LOCAL_IP(10,110,1,203);
  static IPAddress GATEWAY (10,110,1,254);
  static IPAddress SUBNET  (255,255,255,0);
  static IPAddress DNS1    (8,8,8,8);   // Idealmente: DNS corporativo
  static IPAddress DNS2    (4,4,4,4);   // Opcional
#endif

// =================== Backend ===================
// Base del API HTTP (W5500: SOLO HTTP sin TLS)
static const char* API_BASE = "http://10.110.1.9:8084/PTService/ESP32";

// =================== OTA (W5500 NO soporta HTTPS) ===================
// Si no puedes servir .bin por HTTP, desactiva OTA o usa proxy HTTP.
static const char* URL_ACTUALIZA     = "http://10.110.1.9:8084/firmware/ME001.bin";
static const uint16_t OTA_TIMEOUT_MS = 12000;

// =================== Versión ===================
static const char* versionAnterior = "V.1.7";
static const char* enVersion       = "V.1.8";

// =================== Timings ===================
static const uint32_t PERIOD_STATUS_MS = 3000; // intervalo /status
static const uint32_t TIMEOUT_PASO_MS  = 8000; // tiempo máximo de paso (telemetría)

// =================== NTP / Sincronización de hora ===================
// Si no quieres depender de DNS, usa IPs internas (MikroTik).
#define USE_NTP_IP   0   // 1 => IP directa, 0 => pool por nombre
static const char* NTP_IP1  = "10.110.1.1";     // Si tienes NTP interno
static const char* NTP_IP2  = nullptr;          // opcional
static const char* NTP_DNS1 = "es.pool.ntp.org";
static const char* NTP_DNS2 = "pool.ntp.org";
static const char* NTP_DNS3 = "time.google.com";

// Sincronización de fecha vía HTTP (si lo usas en tu time.cpp)
#undef  HTTP_DATE_HOST
#undef  HTTP_DATE_PORT
#undef  HTTP_DATE_PATH
#define HTTP_DATE_HOST "10.110.1.9"
#define HTTP_DATE_PORT 8084
// PATH se construye dinámicamente con DEVICE_ID en tiempo de ejecución.

// =================== Cabeceras/prefijos de QR ===================
// OJO: Estos *no* son endpoints de cliente; se usan para reconocer el origen
// del QR (prefijos/hosts en el contenido escaneado). HTTPS aquí no afecta
// al W5500 porque no se realiza conexión, solo comparación de cadenas.
#define HEAD_ODOO "https://tpv.museoelder.es/pos/ticket/validate"
#define HEAD_TEC  "https://wptpv.museoelder.es/"

// =================== Ficheros locales (LittleFS/SPIFFS) ===================
#ifndef PENDIENTES_PATH
  #define PENDIENTES_PATH "/pendientes.txt"   // reintentos cuando vuelve la red
#endif

#ifndef ENTRADAS_LOCAL_PATH
  #define ENTRADAS_LOCAL_PATH "/entradas.txt" // cache última descarga de entradas
#endif

// =================== Externs (estado global compartido) ===================
// Debug/telemetría
extern uint8_t  debugSerie;

// Estado “máquina/puerta/tickets”
extern String   estadoPuerta;
extern String   estadoMaquina;
extern String   ultimoTicket;

// Flags de control de ciclo
extern int      actualizarFlag, restartFlag;
extern int      activaConecta, activaEntrada, activaSalida;

// Contadores de paso
extern int      pasoActual, contadorPasos, pasosTotales;

// Config/URLs en runtime (permiten ser modificadas)
extern String   serverURL;
extern String   urlActualiza;

// Métricas de latencia de backend
extern unsigned long inicioServidor, finServidor;

// Buffers JSON compartidos
extern String outputInicio, outputEstado, outputTicket, outputPaso;
extern String estadoRecibido, ticketRecibido, pasoRecibido;

// Otros buffers/eventos
extern String recibidoUDP, ultimoPaso;

// Reloj/errores
extern unsigned long  segundosAhora;
extern String errorCode, estadoTicket;
extern int    cambiaPuerta;

// Modo apertura/abortos
extern int  abreContinua, abortarPaso;

// Resultado de validación online + detalles
extern volatile ValidateOutcome g_validateOutcome;
extern String g_lastEd;    // detalle de error del backend (ed)
extern int    g_lastHttp;  // último HTTP status 4xx/5xx si aplica

// ------------------ Mensajería entre tareas ------------------
typedef struct {
  CmdType type;          // orden (validate, pass ok, etc.)
  QRKind  qrKind;        // tipo de QR detectado (si lo alimentas)
  char    payload[128];  // código normalizado (token / ticket_id=...&event_id=... / numérico)
} CmdMsg;
