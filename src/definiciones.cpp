#include "definiciones.hpp"

// =================== Identidad y versión ===================
String DEVICE_ID = "";
String enVersion = "";
String versionAnterior = "";

// =================== Backend / URLs ===================
String serverURL = "";
String urlActualiza = "";

// =================== Estado “máquina/puerta/tickets” ===================
String estadoPuerta = "200";
CmdType estadoMaquina = CMD_READY;
String ultimoTicket = "";
String ultimoPaso = "";

// =================== Flags de control del ciclo ===================
int actualizarFlag = 0;
int restartFlag = 0;
int activaConecta = 1;
int activaEntrada = 0;
int activaSalida = 0;
int abreContinua = 0;
int abortarPaso = 0;

// =================== Contadores de paso ===================
int pasoActual = 0;
int contadorPasos = 0;
int pasosTotales = 0;

// =================== Buffers JSON compartidos ===================
String outputInicio = "";
String outputEstado = "";
String outputTicket = "";
String outputPaso = "";
String estadoRecibido = "";
String ticketRecibido = "";
String pasoRecibido = "";

// =================== Métricas/errores/auxiliares ===================
unsigned long inicioServidor = 0;
unsigned long finServidor = 0;
unsigned long segundosAhora = 0;
String errorCode = "";
String estadoTicket = "";
int cambiaPuerta = 0;

// =================== Resultado validateQR ===================
volatile ValidateOutcome g_validateOutcome = ValidateOutcome::VNONE;
String g_lastEd = "";
int g_lastHttp = 0;

// =================== Portal / webserver ===================
String ssid = "";
String password = "";
String credencialMaestra = "";
bool portalActivo = false;
bool registrado_ap = false;
String errorMessage_ap = "";
bool registrado_eth = false;
String errorMessage_eth = "";

unsigned long tReposo = 0;
unsigned long lastActivityTime_ap = 0;
unsigned long lastActivityTime_eth = 0;
int reintentos = 0;
bool wifiAP_Connected = false;
unsigned long lastWifiCheckMillis = 0;
const long wifiCheckInterval = 5000;                      // ms
const unsigned long INACTIVITY_TIMEOUT_MS_AP = 300000UL;  // 5 min
const unsigned long INACTIVITY_TIMEOUT_MS_ETH = 300000UL; // 5 min

IPAddress softIP = IPAddress(192, 168, 4, 1);

// =================== Portal / ethernetserver ===================
// Instancia única del servidor Ethernet (usa el wrapper para compatibilidad)
MyEthernetServer serverETH(8081);

// =================== Red (W5500) ===================
uint8_t modoRed = 0; // 0 DHCP, 1 IP fija
String sentido = "";

IPAddress IP = IPAddress(0, 0, 0, 0);
IPAddress GATEWAY = IPAddress(0, 0, 0, 0);
IPAddress SUBNET = IPAddress(0, 0, 0, 0);
IPAddress DNS1 = IPAddress(0, 0, 0, 0);
IPAddress DNS2 = IPAddress(0, 0, 0, 0);

// MAC por defecto (la de tu JSON)
uint8_t mac[6] = {0x1C, 0x69, 0x20, 0xE6, 0x21, 0x78};

// =================== Debug ===================
uint8_t debugSerie = 1; // 0 = off, 1 = on
