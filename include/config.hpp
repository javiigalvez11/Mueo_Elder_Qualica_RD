#pragma once
#include <Arduino.h>
#include <WiFi.h>

// === Identidad / constantes ===
static const char* COMERCIO   = "MUSEO_ELDER";
static const char* DEVICE_ID  = "ME002";
static const char* WIFI_SSID  = "Prueba_WIFI";
static const char* WIFI_PASS  = "Qualica_RD";

// IP fija
static IPAddress LOCAL_IP(192,168,88,2);
static IPAddress GATEWAY (192,168,88,1);
static IPAddress SUBNET  (255,255,255,0);
static IPAddress DNS1    (8,8,8,8); // MikroTik
static IPAddress DNS2    (1,1,1,1);      // Google DNS


//Wifi Prueba_WIFI
//static const char* API_BASE   = "http://192.168.88.254:8084/PTService/ESP32";

//Wifi Paythunder-Corp
static const char* API_BASE   = "http://10.110.4.151:8084/PTService/ESP32";
static const uint16_t HTTP_TIMEOUT_MS = 3000;

static const char* URL_ACTUALIZA     = "https://grupocayp.es/wp-content/uploads/ME001.bin";
static const uint16_t OTA_TIMEOUT_MS = 12000;

static const char* versionAnterior = "V.1.7";
static const char* enVersion       = "V.1.8";

// Timings
static const uint32_t PERIOD_STATUS_MS = 3000;
static const uint32_t TIMEOUT_PASO_MS  = 8000;

// Pines
#define PIN_RS485_RX 16
#define PIN_RS485_TX 17
#define PIN_GM65_RX  26
#define PIN_GM65_TX  27
#define PIN_RELE_ENT 21
#define PIN_RELE_SAL 22

#define RS485_BAUD   19200
#define GM65_BAUD    9600
#define MACHINE_ID   0x01

// ====== SOLO DECLARACIONES (extern) ======
extern uint8_t  debugSerie;
extern uint8_t  aperturaMode;

extern String   estadoPuerta;
extern String   ultimoTicket;
extern int      actualizarFlag, restartFlag;
extern int      activaConecta, activaEntrada, activaSalida;

extern int      pasoActual, contadorPasos, pasosTotales;

extern String   serverURL;
extern String   urlActualiza;

extern unsigned long inicioServidor, finServidor;

extern String outputInicio, outputEstado, outputTicket, outputPaso;
extern String estadoRecibido, ticketRecibido, pasoRecibido;

extern String recibidoUDP, ultimoPaso;

extern long   segundosAhora;
extern String errorCode, estadoTicket;
extern int    cambiaPuerta;


extern int  abreContinua, abortarPaso;   

// ------------------------------
// Mensajería entre tareas
// ------------------------------
typedef enum {
  CMD_NONE = 0,
  CMD_VALIDATE,       // validar barcode
  CMD_PASS_OK,        // notificar paso OK
  CMD_PASS_TIMEOUT,   // notificar que no pasó nadie (timeout)
  CMD_SLAVE_EXIT      // apertura para salida
} CmdType;

typedef struct {
  CmdType type;
  char  payload[64];    // barcode
} CmdMsg;
