#include "config.hpp"

// --- flags & estado general ---
uint8_t  debugSerie      = 1;
uint8_t  aperturaMode    = 0;   // 0=rele, 1=RS485

String   estadoPuerta    = "200";
String   ultimoTicket;

// flags de control
int actualizarFlag = 0;
int restartFlag    = 0;
int activaConecta  = 1;
int activaEntrada  = 0;
int activaSalida   = 0;

// contadores/pasos
int pasoActual     = 0;
int contadorPasos  = 0;
int pasosTotales   = 0;

// red / http
String serverURL    = API_BASE;       // <- inicializa con la constante
String urlActualiza = URL_ACTUALIZA;  // <- idem

// buffers JSON compartidos
String outputInicio, outputEstado, outputTicket, outputPaso;
String estadoRecibido, ticketRecibido, pasoRecibido;

// tiempos que usas para medir latencias
unsigned long inicioServidor = 0;
unsigned long finServidor    = 0;

// otros que usa la lógica
int  abreContinua = 0;   // <- NOMBRE CORRECTO (no “abrirContinua”)
int  abortarPaso  = 0;
String recibidoUDP;
String ultimoPaso;

long   segundosAhora = 0;
String errorCode     = "";
String estadoTicket  = "";
int    cambiaPuerta  = 0;


