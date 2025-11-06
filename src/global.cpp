#include "config.hpp"

// ============================================================================
// DEFINICIÓN DE ESTADO GLOBAL (evita definiciones múltiples)
// ----------------------------------------------------------------------------
// Mantiene compatibilidad con el resto del código. Si en el futuro quieres
// aislar estado, podemos encapsular en una struct/singleton y pasar punteros.
// ============================================================================

// --- flags & estado general ---
uint8_t  debugSerie      = 1;

// Estado principal
String   estadoPuerta    = "200";
String   estadoMaquina   = "CMD_READY";
String   ultimoTicket;
QRKind   ultimoKindQR    = QR_UNKNOWN;

// flags de control
int actualizarFlag = 0;
int restartFlag    = 0;
int activaConecta  = 1;  // /status activo
int activaEntrada  = 0;  // backend ha autorizado ENTRADA (201)
int activaSalida   = 0;  // backend ha autorizado SALIDA  (202)

// contadores/pasos (reseteados por applyStatus/handleValidationError)
int pasoActual     = 0;
int contadorPasos  = 0;
int pasosTotales   = 0;

// red / http (modificables en runtime si lo necesitas)
String serverURL    = API_BASE;
String urlActualiza = URL_ACTUALIZA;

// buffers JSON compartidos (reutilizados para minimizar allocs)
String outputInicio, outputEstado, outputTicket, outputPaso;
String estadoRecibido, ticketRecibido, pasoRecibido;

// tiempos para medir latencia de backend (opcional)
unsigned long inicioServidor = 0;
unsigned long finServidor    = 0;

// otros que usa la lógica
int  abreContinua = 0;     // toggle de apertura continua (comando 203)
int  abortarPaso  = 0;     // bandera para abortar paso en curso
String recibidoUDP;
String ultimoPaso;

unsigned long  segundosAhora = 0; // si sincronizas con NTP/HTTP
String errorCode     = "";        // para reportFailure()
String estadoTicket  = "";        // si lo usas en UI/log
int    cambiaPuerta  = 0;         // si lo usas para triggers locales

// Resultado de validateQR
volatile ValidateOutcome g_validateOutcome = VNONE;
String g_lastEd = "";     // detalle textual 'ed' del backend (p.ej., TICKET_ALREADY_USED)
int    g_lastHttp = 0;    // último status 4xx/5xx recibido en validateQR
