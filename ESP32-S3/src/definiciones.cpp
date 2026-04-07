#include <Arduino.h>
#include "definiciones.hpp"
#include "types.hpp"
#include "ethernet_wrapper.hpp"


//===============================================================
//==================== CONFIGURACIÓN PÚBLICA ====================
//===============================================================
    String DEVICE_ID="ME012"; //Nombre de el dispositivo
    String credencialMaestra = "Qualicard";
    uint8_t MAC[6];

    // === Debug ===
    uint8_t debugSerie = 1; // 0 = off, 1 = on

    // Modo de apetura de las puertas
    uint8_t sentidoApertura = 1; //0 => Entrada se realiza con leftOpen() , 1 => Entreda se realiza con rightOpen()
    uint8_t modoApertura = 0; //0 => RS485, 1 => Relés
    uint8_t modoPasillo = 0;  //0 => Vega, 1 => canopu, 2 => Arturus
    uint32_t entradasTotales = 0;
    uint32_t salidasTotales = 0;

    // === Red (WIFI) ===
    String ssidComercio = "Paythunder-Corp";
    String passwordComercio ="Fr1k4d4_QRD_$";

    // === Red (WIFI - AP) === 
    String ssidAP = "ME012_WIFI";
    String passwordAP ="Qualicard";
    bool portalApActivo = false;
    uint8_t fallosConsecutivos = 0; // Contador Anti-rebote

    uint8_t conexionRed = 0; //0 => WIFI, 1 => Ethernet(W5500). 
    uint8_t modoRed = 0; // 0 DHCP, 1 IP fija

    IPAddress IP = IPAddress(10, 110, 1, 130);
    IPAddress GATEWAY = IPAddress(10, 110, 1, 10);
    IPAddress SUBNET = IPAddress(255, 255, 255, 0);
    IPAddress DNS1 = IPAddress(8, 8, 8, 8);
    IPAddress DNS2 = IPAddress(4, 4, 4, 4);

    //String serverURL = "http://10.110.4.220:8080/api";
    //String serverURL = "http://192.168.8.200:8080/api";  
    //String serverURL = "http://validaciones.museoelder.es:8537/PTService/ESP32"; 
    String serverURL = "http://10.110.4.9:8084/QRDService/prueba";
    //String serverURL = "http://10.110.1.9:8084/QRDService/api";
    String urlActualiza = "https://grupocayp.es/wp-content/uploads/ME012.bin";


    // =================== Versión ===================
    String enVersion = "3.0";
    String versionAnterior = "2.0";

//===============================================================
//==================== CONFIGURACIÓN PLACA ====================
//===============================================================
    uint16_t p_machineId      = 1;  // 0
    uint8_t  p_openingMode    = 1;  // 1
    uint16_t p_waitTime       = 8;  // 2
    uint8_t  p_voiceLeft      = 3;  // 3
    uint8_t  p_voiceRight     = 5;  // 4
    uint8_t  p_voiceVol       = 12; // 5
    uint8_t  p_masterSpeed    = 10; // 6
    uint8_t  p_slaveSpeed     = 10; // 7
    uint8_t  p_debugMode      = 0;  // 8
    uint8_t  p_decelRange     = 10; // 9
    uint8_t  p_selfTestSpeed  = 3;  // 10
    uint8_t  p_passageMode    = 0;  // 11
    uint8_t  p_closeControl   = 2;  // 12
    uint8_t  p_singleMotor    = 0;  // 13
    uint8_t  p_language       = 4;  // 14 (Español)
    uint8_t  p_irRebound      = 1;  // 15
    uint8_t  p_pinchSens      = 5;  // 16
    uint8_t  p_reverseEntry   = 1;  // 17
    uint8_t  p_turnstileType  = 0;  // 18
    uint8_t  p_emergencyDir   = 2;  // 19
    uint8_t  p_motorResist    = 5;  // 20
    uint8_t  p_intrusionVoice = 1;  // 21
    uint16_t p_irDelay        = 5;  // 22
    uint8_t  p_motorDir       = 1;  // 23
    uint8_t  p_clutchLock     = 0;  // 24
    uint8_t  p_hallType       = 0;  // 25
    uint8_t  p_signalFilter   = 3;  // 26
    uint8_t  p_cardInside     = 0;  // 27
    uint8_t  p_tailgateAlarm  = 2;  // 28
    uint8_t  p_limitDev       = 3;  // 29
    uint8_t  p_pinchFree      = 1;  // 30
    uint8_t  p_memoryFree     = 0;  // 31
    uint8_t  p_slipMaster     = 0;  // 32
    uint8_t  p_slipSlave      = 0;  // 33
    uint8_t  p_irLogicMode    = 0;  // 34
    uint8_t  p_lightMaster    = 1;  // 35
    uint8_t  p_lightSlave     = 2;  // 36

//===============================================================
//==================== CONFIGURACIÓN HARDWARE ===================
//===============================================================

// =================== Estado “máquina/puerta/tickets” ===================
    int estadoPuerta = 200;
    CmdType estadoMaquina = CMD_READY;
    String ultimoTicket = "";
    String ultimoPaso = "";
    bool iniciOk = true;

// =================== Flags de control del ciclo ===================
    int actualizarFlag = 0;
    int restartFlag = 0;
    int activaConecta = 1;
    bool errorNotificado = false;
    int bloqueoLector = 0; // 0 = no bloqueado, 1 = bloqueado de entrada, 2 = bloqueado de salida

// =================== Contadores de paso ===================
    int pasosActuales = 0;
    int contadorPasos = 0;
    int pasosTotales = 0;

// =================== Buffers JSON compartidos ===================
    String outputInicio = "";
    String outputEstado = "";
    String outputTicket = "";
    String outputPaso = "";
    String outputReportFailure = "";
    String estadoRecibido = "";
    String ticketRecibido = "";
    String pasoRecibido = "";

// =================== Métricas/errores/auxiliares ===================
    unsigned long inicioServidor = 0;
    unsigned long finServidor = 0;
    unsigned long segundosAhora = 0;
    QRKind ultimoTipoQR = QR_UNKNOWN;
    String errorCode = "";
    String estadoTicket = "";
    int cambiaPuerta = 0;

// =================== Resultado validateQR ===================
    volatile ValidateOutcome g_validateOutcome = ValidateOutcome::VNONE;
    String g_lastEd = "";
    int g_lastHttp = 0;


// =================== Portal / ethernetserver ===================
// Instancia única del servidor Ethernet (usa el wrapper para compatibilidad)
    MyEthernetServer serverETH(8081);

    bool portalActivo = false;
    bool registrado_eth = false;
    String errorMessage_eth = "";

    unsigned long tReposo = 0;
    unsigned long lastActivityTime_eth = 0;
    int reintentos = 0;
    bool wifiAP_Connected = false;
    unsigned long lastWifiCheckMillis = 0;
    const long wifiCheckInterval = 5000;                      // ms
    const unsigned long INACTIVITY_TIMEOUT_MS_ETH = 300000UL; // 5 min

//=================== Variables RS485 =======================
    uint8_t  startPos=0, version_number=0, machineNumber_ret=0;
    uint8_t  faultEvent=0, gateStatus=0, alarmEvent=0;
    uint32_t leftCount=0, rightCount=0;
    uint8_t  infraredStatus=0, commandExecStatus=0, powerSupplyVolt=0, undef1=0, undef2=0, checkSum=0;