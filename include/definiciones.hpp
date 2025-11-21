#ifndef DEFINICIONES_HPP
#define DEFINICIONES_HPP
#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <stdint.h>
#include "ethernet_wrapper.hpp"
#include "types.hpp"

// ============================================================================
// CONFIGURACIÓN GLOBAL Y ESTADO COMPARTIDO — Museo Elder (ME001)
// ============================================================================
// Reglas:
// - Aquí SOLO declaraciones (extern) y constexpr.
// - Las definiciones reales están en definiciones.cpp.
// ============================================================================

// =================== Identidad y versión ===================
extern String DEVICE_ID;       // Debe coincidir con nombrePlaca
extern String enVersion;       // Versión actual de firmware (p.ej. "V.1.8")
extern String versionAnterior; // Versión previa (si la usas en UI)

// =================== Backend / URLs ===================
extern String serverURL;    // Base del API
extern String urlActualiza; // URL .bin para OTA (W5500 => http://)

// =================== OTA (timeouts) ===================
constexpr uint16_t OTA_TIMEOUT_MS = 12000;

// =================== Timings — latidos/telemetría ===================
constexpr uint32_t PERIOD_STATUS_MS = 3000; // /status periódico
constexpr uint32_t TIMEOUT_PASO_MS = 8000;  // tiempo máximo de paso

// =================== NTP / Horario ===================
// Si usas DNS NTP:
constexpr bool USE_NTP_IP = false; // true => usar IP directa, false => DNS
constexpr const char *NTP_DNS1 = "es.pool.ntp.org";
constexpr const char *NTP_DNS2 = "pool.ntp.org";
constexpr const char *NTP_DNS3 = "time.google.com";
// Si usas IP NTP (solo si USE_NTP_IP = true)
constexpr const char *NTP_IP1 = "10.110.1.1";
constexpr const char *NTP_IP2 = nullptr;

// =================== Sincronía por cabecera HTTP Date ===================
constexpr const char *HTTP_DATE_HOST = "validaciones.museoelder.es";
constexpr uint16_t HTTP_DATE_PORT = 8537;

// =================== Cabeceras/prefijos de QR (detección de origen) ===================
constexpr const char *HEAD_ODOO = "https://tpv.museoelder.es/pos/ticket/validate";
constexpr const char *HEAD_TEC = "https://wptpv.museoelder.es/";

// =================== Ficheros locales (LittleFS) ===================
#ifndef PENDIENTES_PATH
#define PENDIENTES_PATH "/pendientes.txt"
#endif
#ifndef ENTRADAS_LOCAL_PATH
#define ENTRADAS_LOCAL_PATH "/entradas.txt"
#endif

// =================== Estado “máquina/puerta/tickets” ===================
extern String estadoPuerta;   // "200"/"201"/"203"/...
extern CmdType estadoMaquina; // "CMD_READY", "CMD_VALIDATE_IN", etc.
extern String ultimoTicket;   // último QR enviado a validar
extern String ultimoPaso;     // último QR confirmado como PASS_OK

// =================== Flags de control del ciclo ===================
extern int actualizarFlag; // 1 => iniciar OTA
extern int restartFlag;    // 1 => reset
extern int activaConecta;  // habilita /status periódico
extern int activaEntrada;  // backend autorizó ENTRADA (201)
extern int activaSalida;   // backend autorizó SALIDA  (202)
extern int abreContinua;   // 1 => puerta abierta continua (203)
extern int abortarPaso;    // 1 => abortar paso actual

// =================== Contadores de paso ===================
extern int pasoActual;    // índice del paso actual (telemetría)
extern int contadorPasos; // contador acumulado
extern int pasosTotales;  // pasos autorizados por backend

// =================== Buffers JSON compartidos ===================
extern String outputInicio, outputEstado, outputTicket, outputPaso;
extern String estadoRecibido, ticketRecibido, pasoRecibido;

// =================== Métricas/errores/auxiliares ===================
extern unsigned long inicioServidor, finServidor; // medir latencia HTTP
extern unsigned long segundosAhora;
extern String errorCode;
extern String estadoTicket;
extern int cambiaPuerta;

// =================== Resultado validateQR ===================
extern volatile ValidateOutcome g_validateOutcome;
extern String g_lastEd; // "TICKET_ALREADY_USED", etc.
extern int g_lastHttp;  // último HTTP status (4xx/5xx)

// =================== Mensajería entre tareas ===================
typedef struct
{
  CmdType type;
  QRKind qrKind;
  char payload[128];
} CmdMsg;

// =================== Variables del portal / webserver ===================
extern String ssid;              // SSID del AP de mantenimiento
extern String password;          // Contraseña del AP de mantenimiento
extern String credencialMaestra; // Password para login web
extern bool portalActivo;        // AP/portal activo
extern bool registrado_ap;       // sesión web validada (AP)
extern String errorMessage_ap;   // mensaje UI web

extern unsigned long tReposo;
extern unsigned long lastActivityTime_ap;
extern const unsigned long INACTIVITY_TIMEOUT_MS_AP;

extern int reintentos;
extern bool wifiAP_Connected;
extern unsigned long lastWifiCheckMillis;
extern const long wifiCheckInterval;

// Pin que activa el portal al arrancar (puente a GND)
constexpr int AP_PIN = 21;

// IP del AP
extern IPAddress softIP;

// Servidor HTTP sobre Ethernet (puerto 80)
extern MyEthernetServer serverETH;

// Variables menús, web y sesión
extern bool registrado_eth;
extern unsigned long lastActivityTime_eth;
extern const unsigned long INACTIVITY_TIMEOUT_MS_ETH;
extern String errorMessage_eth;

// =================== Config de red de equipo (W5500) ===================
// Estas se cargan desde /config.json:
extern uint8_t modoRed; // 0 => DHCP, 1 => IP fija
extern String sentido;  // "Entrada"/"Salida"

// IPs de red por cable (W5500)
extern IPAddress IP;
extern IPAddress GATEWAY;
extern IPAddress SUBNET;
extern IPAddress DNS1;
extern IPAddress DNS2;

// MAC binaria para Ethernet
extern uint8_t mac[6];

// =================== Debug global ===================
extern uint8_t debugSerie;

#endif // DEFINICIONES_HPP
