#ifndef DEFINICIONES_HPP
#define DEFINICIONES_HPP

#pragma once
#include <Arduino.h>
#include "types.hpp"
#include "ethernet_wrapper.hpp"

//===============================================================
//==================== CONFIGURACIÓN PÚBLICA ====================
//===============================================================
    // =================== Identidad y versión ===================
    extern String DEVICE_ID;       // Debe coincidir con nombrePlaca
    extern String credencialMaestra; // Credencial maestra (QR/UID)
   
    // Modo de apetura de las puertas
    extern uint8_t sentidoApertura; //0 => Entrada se realiza con leftOpen() , 1 => Entreda se realiza con rightOpen()
    extern uint8_t modoApertura; //0 => RS485, 1 => Relés
    extern uint8_t modoPasillo;  //0 => Vega, 1 => canopu, 3 => Arturus
    extern uint32_t entradasTotales; // Contador total de entradas
    extern uint32_t salidasTotales;  // Contador total de salidas

    extern String enVersion;       // Versión actual de firmware (p.ej. "V.1.8")
    extern String versionAnterior; // Versión previa (si la usas en UI)

    // =================== Backend / URLs ===================
    extern String serverURL;    // Base del API
    extern String urlActualiza; // URL .bin para OTA (W5500 => http://)

    // =================== Config de red de equipo (W5500) ===================
    // Estas se cargan desde /config.json:
    extern uint8_t conexionRed; // 0 => WIFI, 1 => Ethernet(W5500).
    extern uint8_t modoRed; // 0 => DHCP, 1 => IP fija

    extern String ssidComercio;
    extern String passwordComercio;

    extern String ssidAP;
    extern String passwordAP;
    extern bool portalApActivo;
    extern uint8_t fallosConsecutivos; // Contador Anti-rebote

    // IPs de red por cable (W5500)
    extern IPAddress IP;
    extern IPAddress GATEWAY;
    extern IPAddress SUBNET;
    extern IPAddress DNS1;
    extern IPAddress DNS2;

    // MAC binaria para Ethernet
    extern uint8_t MAC[6];

    // =================== Debug global ===================
    extern uint8_t debugSerie;

// Parámetros técnicos del torno (RS485)
    extern uint16_t p_machineId;      // 0
    extern uint8_t  p_openingMode;    // 1
    extern uint16_t p_waitTime;       // 2
    extern uint8_t  p_voiceLeft;      // 3
    extern uint8_t  p_voiceRight;     // 4
    extern uint8_t  p_voiceVol;       // 5
    extern uint8_t  p_masterSpeed;    // 6
    extern uint8_t  p_slaveSpeed;     // 7
    extern uint8_t  p_debugMode;      // 8
    extern uint8_t  p_decelRange;     // 9
    extern uint8_t  p_selfTestSpeed;  // 10
    extern uint8_t  p_passageMode;    // 11
    extern uint8_t  p_closeControl;   // 12
    extern uint8_t  p_singleMotor;    // 13
    extern uint8_t  p_language;       // 14
    extern uint8_t  p_irRebound;      // 15
    extern uint8_t  p_pinchSens;      // 16
    extern uint8_t  p_reverseEntry;   // 17
    extern uint8_t  p_turnstileType;  // 18
    extern uint8_t  p_emergencyDir;   // 19
    extern uint8_t  p_motorResist;    // 20
    extern uint8_t  p_intrusionVoice; // 21
    extern uint16_t p_irDelay;        // 22
    extern uint8_t  p_motorDir;       // 23
    extern uint8_t  p_clutchLock;     // 24
    extern uint8_t  p_hallType;       // 25
    extern uint8_t  p_signalFilter;   // 26
    extern uint8_t  p_cardInside;     // 27
    extern uint8_t  p_tailgateAlarm;  // 28
    extern uint8_t  p_limitDev;       // 29
    extern uint8_t  p_pinchFree;      // 30
    extern uint8_t  p_memoryFree;     // 31
    extern uint8_t  p_slipMaster;     // 32
    extern uint8_t  p_slipSlave;      // 33
    extern uint8_t  p_irLogicMode;    // 34
    extern uint8_t  p_lightMaster;    // 35
    extern uint8_t  p_lightSlave;     // 36



//===========================================================================
//======================== Varibles de Entorno ==============================
//===========================================================================

    // =================== Estado “máquina/puerta/tickets” ===================
    extern int estadoPuerta;   // "200"/"201"/"203"/...
    extern CmdType estadoMaquina; // "READY", "VALIDATE_IN", etc.
    extern String ultimoTicket;   // último QR enviado a validar
    extern String ultimoPaso;     // último QR confirmado como PASS_OK
    extern bool iniciOk;          // indica si el ciclo principal ha iniciado correctamente (después de OTA, etc.)

    // =================== Flags de control del ciclo ===================
    extern int actualizarFlag; // 1 => iniciar OTA
    extern int restartFlag;    // 1 => reset
    extern int activaConecta;  // habilita /status periódico
    extern bool errorNotificado; // si ya se notificó error al backend en este ciclo
    extern int bloqueoLector; // 0 = no bloqueado, 1 = bloqueado de entrada, 2 = bloqueado de salida


    // =================== Contadores de paso ===================
    extern int pasosActuales;    // índice del paso actual (telemetría)
    extern int contadorPasos; // contador acumulado
    extern int pasosTotales;  // pasos autorizados por backend

    // =================== Buffers JSON compartidos ===================
    extern String outputInicio, outputEstado, outputTicket, outputPaso, outputReportFailure;
    extern String estadoRecibido, ticketRecibido, pasoRecibido;

    // =================== Métricas/errores/auxiliares ===================
    extern unsigned long inicioServidor, finServidor; // medir latencia HTTP
    extern unsigned long segundosAhora;
    extern QRKind ultimoTipoQR; // para telemetría, debugging y UI (p.ej. mostrar icono distinto para TICKET vs CREDENCIAL)
    extern String errorCode;
    extern String estadoTicket;
    extern int cambiaPuerta;

    // =================== Resultado validateQR ===================
    extern volatile ValidateOutcome g_validateOutcome;
    extern String g_lastEd; // "TICKET_ALREADY_USED", etc.
    extern int g_lastHttp;  // último HTTP status (4xx/5xx)

    // =================== OTA (timeouts) ===================
    constexpr uint16_t OTA_TIMEOUT_MS = 12000;

    // =================== Timings — latidos/telemetría ===================
    constexpr uint32_t PERIOD_STATUS_MS = 3000; // /status periódico
    constexpr uint32_t SERVER_TIMEOUT = 5000;
    constexpr uint32_t PASO_TIMEOUT = 8000;

    // Servidor HTTP sobre Ethernet (puerto 80)
    extern MyEthernetServer serverETH;

    // Variables menús, web y sesión
    extern bool registrado_eth;
    extern unsigned long lastActivityTime_eth;
    extern const unsigned long INACTIVITY_TIMEOUT_MS_ETH;
    extern String errorMessage_eth;

    //=================== Variables RS485 =======================
    extern uint8_t  startPos, version_number, machineNumber_ret;
    extern uint8_t  faultEvent, gateStatus, alarmEvent;
    extern uint32_t leftCount, rightCount;
    extern uint8_t  infraredStatus, commandExecStatus, powerSupplyVolt, undef1, undef2, checkSum;

    // =================== Sincronía por cabecera HTTP Date ===================
    constexpr const char *HTTP_DATE_HOST = "validaciones.museoelder.es";
    constexpr uint16_t HTTP_DATE_PORT = 8537;

    // =================== Ficheros locales (LittleFS) ===================
    #ifndef PENDIENTES_PATH
    #define PENDIENTES_PATH "/pendientes.txt"
    #endif
    #ifndef ENTRADAS_LOCAL_PATH
    #define ENTRADAS_LOCAL_PATH "/entradas.txt"
    #endif

//===============================================================
//==================== CONFIGURACIÓN HARDWARE ===================
//===============================================================


#endif // DEFINICIONES_HPP
