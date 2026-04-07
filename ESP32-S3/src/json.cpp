#include "json.hpp"
#include "definiciones.hpp"
#include "rele.hpp"
#include "logBuf.hpp"
#include "types.hpp"
#include "RS485.hpp"
#include "http.hpp"

// ========================= Helpers de Robustez =========================

// Lee un entero sin importar si viene como número o como string
static int getIntFlex(const JsonDocument &d, const char *key)
{
    JsonVariantConst v = d[key]; // Usamos JsonVariantConst para v7
    if (v.isNull())
        return 0;
    if (v.is<int>() || v.is<long>())
        return v.as<int>();
    if (v.is<const char *>())
        return String(v.as<const char *>()).toInt();
    return 0;
}

// Busca una cadena en múltiples posibles llaves (ej: "ec" o "EC") sin usar containsKey
static String getStringFlex(const JsonDocument &d, const char *key1, const char *key2)
{
    if (!d[key1].isNull())
        return d[key1].as<String>();
    if (!d[key2].isNull())
        return d[key2].as<String>();
    return "";
}

static inline int toIntSafe(const String &s, int dflt)
{
    if (s.length() == 0)
        return dflt;
    char *end = nullptr;
    long v = strtol(s.c_str(), &end, 10);
    return (end && *end == '\0') ? (int)v : dflt;
}

// Detecta "ok" o "success" ignorando mayúsculas y probando llaves "r" y "R"
static inline bool json_is_ok(const JsonDocument &d)
{
    String r = "";
    if (!d["r"].isNull())
        r = d["r"].as<String>();
    else if (!d["R"].isNull())
        r = d["R"].as<String>();

    r.trim();
    r.toLowerCase();
    return (r == "ok" || r == "success");
}

// Obtiene el status buscando en "s", "S", "status" o "STATUS"
static inline int get_status_as_int(const JsonDocument &d)
{
    int s = getIntFlex(d, "s");
    if (s == 0)
        s = getIntFlex(d, "status");
    if (s == 0)
        s = getIntFlex(d, "S");
    return s;
}

static uint8_t parseHexByte(const char *str)
{
    if (!str || str[0] == '\0')
        return 0;
    return (uint8_t)strtoul(str, nullptr, 16);
}

// ========================= Control de Ciclo =========================

void resetCycleReady()
{
    activaConecta = 1;
    ultimoTicket = "";
    ultimoPaso = "";
    pasosActuales = 0;
    contadorPasos = 0;
    estadoMaquina = CMD_READY;
    estadoPuerta = 200;
}

// ========================= Mapeos de Comando =========================

static inline const char *ec_to_str(CmdType c)
{
    switch (c)
    {
    case CMD_READY:
        return "CMD_READY";
    case CMD_OPEN_CONTINUOUS:
        return "CMD_OPEN_CONTINUOUS";
    case CMD_VALIDATE_IN:
        return "CMD_VALIDATE_IN";
    case CMD_VALIDATE_OUT:
        return "CMD_VALIDATE_OUT";
    case CMD_PASS_OK:
        return "CMD_PASS_OK";
    case CMD_PASS_TIMEOUT:
        return "CMD_PASS_TIMEOUT";
    case CMD_PASS_IN:
        return "CMD_PASS_IN";
    case CMD_PASS_OUT:
        return "CMD_PASS_OUT";
    case CMD_ABORT:
        return "CMD_ABORT";
    case CMD_BAD_REQUEST:
        return "CMD_BAD_REQUEST";
    case CMD_UNAUTHORIZED:
        return "CMD_UNAUTHORIZED";
    case CMD_ALREADY_USED:
        return "CMD_ALREADY_USED";
    case CMD_RESTART:
        return "CMD_RESTART";
    case CMD_UPDATE:
        return "CMD_UPDATE";
    case CMD_FAIL_REPORT:
        return "CMD_FAIL_REPORT";
    default:
        return "CMD_NONE";
    }
}

static CmdType cmd_from_ec_string(const String &ec)
{
    String s = ec;
    s.toUpperCase();
    if (s == "CMD_READY")
        return CMD_READY;
    if (s == "CMD_OPEN_CONTINUOUS")
        return CMD_OPEN_CONTINUOUS;
    if (s == "CMD_VALIDATE_IN")
        return CMD_VALIDATE_IN;
    if (s == "CMD_VALIDATE_OUT")
        return CMD_VALIDATE_OUT;
    if (s == "CMD_PASS_OK")
        return CMD_PASS_OK;
    if (s == "CMD_PASS_TIMEOUT")
        return CMD_PASS_TIMEOUT;
    if (s == "CMD_PASS_IN" || s == "PASS_IN")
        return CMD_PASS_IN;
    if (s == "CMD_PASS_OUT" || s == "PASS_OUT")
        return CMD_PASS_OUT;
    if (s == "CMD_ABORT")
        return CMD_ABORT;
    if (s == "CMD_RESTART")
        return CMD_RESTART;
    if (s == "CMD_UPDATE")
        return CMD_UPDATE;
    if (s == "CMD_FAIL_REPORT")
        return CMD_FAIL_REPORT;
    return CMD_NONE;
}

// ========================= Lógica de Aplicación =========================

static void applyStatusLogic(int s, const String &ec)
{
    estadoPuerta = s;
    CmdType nuevoEstado = cmd_from_ec_string(ec);

    switch (s)
    {
    case 200:
        if (nuevoEstado == CMD_FAIL_REPORT)
        {
            nuevoEstado = CMD_READY;
            resetCycleReady();
        }
        break;
    case 201:
        nuevoEstado = CMD_VALIDATE_IN;
        break;
    case 202:
        nuevoEstado = CMD_VALIDATE_OUT;
        break;
    case 203:
        nuevoEstado = CMD_PASS_IN;
        break;
    case 204:
        nuevoEstado = CMD_PASS_OUT;
        break;
    case 205:
        nuevoEstado = CMD_PASS_OK;
        break;
    case 206:
        nuevoEstado = CMD_PASS_TIMEOUT;
        break;
    case 300:
        nuevoEstado = CMD_ABORT;
        if (!modoApertura)
            RS485::closeGate(MACHINE_ID);
        else
            rele::close();
        resetCycleReady();
        break;
    case 305:
        restartFlag = 1;
        break;
    case 310:
        actualizarFlag = 1;
        break;
    case 400:
    case 401:
    case 402:
    case 409:
        resetCycleReady();
        break;
    }
    if (nuevoEstado != CMD_NONE)
        estadoMaquina = nuevoEstado;
}

static void procesarComandoHardware(const JsonDocument &doc)
{
    if (pasosActuales > pasosTotales && pasosTotales != 0)
        return;

    uint8_t cmd = parseHexByte(doc["CMD"] | doc["cmd"] | "0x00");
    uint8_t d0 = parseHexByte(doc["DATA0"] | doc["data0"] | "0x00");
    uint8_t d1 = parseHexByte(doc["DATA1"] | doc["data1"] | "0x00");

    if (cmd == 0x00)
        return;

    switch (cmd)
    {
    case 0x10:
        RS485::queryDeviceStatus(MACHINE_ID);
        break;
    case 0x20:
        (sentidoApertura == 0) ? RS485::resetLeftCount(MACHINE_ID) : RS485::resetRightCount(MACHINE_ID);
        break;
    case 0x21:
        (sentidoApertura == 0) ? RS485::resetRightCount(MACHINE_ID) : RS485::resetLeftCount(MACHINE_ID);
        break;
    case 0x35:
        RS485::resetDevice(MACHINE_ID);
        break;
    case 0x80: // Entrada
        if (modoApertura == 0)
        {
            (sentidoApertura == 0) ? RS485::leftOpen(MACHINE_ID, d0) : RS485::rightOpen(MACHINE_ID, d0);
        }
        else
            rele::openEntry();
        break;
    case 0x82: // Salida
        if (modoApertura == 0)
        {
            (sentidoApertura == 0) ? RS485::rightOpen(MACHINE_ID, d0) : RS485::leftOpen(MACHINE_ID, d0);
        }
        else
            rele::openExit();
        break;
    case 0x84:
        RS485::closeGate(MACHINE_ID);
        break;
    case 0x8F:
        RS485::disablePassageRestriccion(MACHINE_ID);
        break;
    }
}

// ========================= Serializadores =========================
void serializaInicio()
{
    JsonDocument doc;
    doc["R"] = "OK";
    doc["ID"] = DEVICE_ID;                // ID del dispositivo
    doc["S"] = estadoPuerta;              // Estado numérico de la puerta
    doc["EC"] = ec_to_str(estadoMaquina); // Estado de máquina como string
    doc["V"] = enVersion;                 // Versión del firmware
    doc["MP"] = modoPasillo;              // Modo de pasillo 0--> Vega, 1--> canopu, 2--> Arturus
    doc["MA"] = modoApertura;             // Modo de apertura 0--> RS485, 1--> Relés
    doc["CE"] = entradasTotales;
    doc["CS"] = salidasTotales;
    doc["MR"] = modoRed;     // Modo de red 0--> DHCP, 1--> IP fija
    doc["CR"] = conexionRed; // Conexión de red 0--> WIFI, 1--> Ethernet(W5500)
    doc["IP"] = IP.toString();
    if (modoRed == 1)
    {
        doc["P"] = "8081"; // Puerto fijo para comunicación con tablet
    }else
    {
        doc["P"] = "8080"; // Indica que el puerto se asigna dinámicamente
    }

    outputInicio.remove(0);
    serializeJson(doc, outputInicio);
}

void serializaEstado()
{
    JsonDocument doc;
    doc["r"] = "OK";
    doc["id"] = DEVICE_ID;
    doc["status"] = estadoPuerta;
    doc["ec"] = ec_to_str(estadoMaquina);
    outputEstado.remove(0);
    serializeJson(doc, outputEstado);
}

void serializaQR()
{
    JsonDocument doc;
    doc["r"] = "OK";
    doc["id"] = DEVICE_ID;
    doc["status"] = estadoPuerta;
    doc["ec"] = ec_to_str(estadoMaquina);
    doc["barcode"] = ultimoTicket;
    outputTicket.remove(0);
    serializeJson(doc, outputTicket);
}

void serializaPaso()
{
    JsonDocument doc;
    doc["r"] = "OK";
    doc["id"] = DEVICE_ID;
    doc["status"] = estadoPuerta;
    doc["ec"] = ec_to_str(estadoMaquina);
    doc["barcode"] = ultimoTicket;
    doc["np"] = pasosActuales;
    doc["nt"] = pasosTotales;
    outputPaso.remove(0);
    serializeJson(doc, outputPaso);
}

void serializaReportFailure()
{
    JsonDocument doc;
    doc["r"] = "OK";
    doc["id"] = DEVICE_ID;
    doc["s"] = "402";
    doc["ec"] = "FAIL_REPORT";
    doc["fallo"] = faultEvent;
    doc["puertas"] = gateStatus;
    doc["alarma"] = alarmEvent;
    doc["ce"] = leftCount;
    doc["cs"] = rightCount;
    doc["voltaje"] = powerSupplyVolt;

    outputReportFailure.remove(0);
    serializeJson(doc, outputReportFailure);
}

// ========================= Deserializadores =========================

void descifraEstado()
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, estadoRecibido);
    if (err)
        return;

    applyStatusLogic(get_status_as_int(doc), getStringFlex(doc, "ec", "EC"));
    procesarComandoHardware(doc);
    estadoRecibido = "";
}

void descifraQR()
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, ticketRecibido);
    if (error)
    {
        g_validateOutcome = VERROR;
        return;
    }

    int status = get_status_as_int(doc);
    String ec = getStringFlex(doc, "ec", "EC");

    if (json_is_ok(doc))
    {
        pasosTotales = getIntFlex(doc, "nt");
        pasosActuales = getIntFlex(doc, "np");

        activaConecta = 0; // Bloqueamos latido para que no se resetee el proceso
        applyStatusLogic(status, ec);

        // Lógica de éxito: aceptamos el código original o el de paso
        if (status == 203 || ec == "CMD_PASS_IN" || ec == "CMD_VALIDATE_IN")
        {
            g_validateOutcome = VAUTH_IN;
            g_lastEd = "OK";
        }
        else if (status == 204 || ec == "CMD_PASS_OUT" || ec == "CMD_VALIDATE_OUT")
        {
            g_validateOutcome = VAUTH_OUT;
            g_lastEd = "OK";
        }
        else
        {
            g_validateOutcome = VDENIED;
            g_lastEd = "UNAUTHORIZED";
        }
    }
    else
    {
        g_validateOutcome = VDENIED;
        g_lastEd = "SERVER_REJECTED";
        applyStatusLogic(status, ec);
    }

    if (debugSerie)
    {
        Serial.printf("[JSON] Validacion: %s | Status: %d | Pasos: %d/%d\n",
                      g_lastEd.c_str(), status, pasosActuales, pasosTotales);
    }
    ticketRecibido = "";
}

void descifraPaso()
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, pasoRecibido);
    if (err)
        return;

    pasosTotales = getIntFlex(doc, "nt");
    pasosActuales = getIntFlex(doc, "np");

    applyStatusLogic(get_status_as_int(doc), getStringFlex(doc, "ec", "EC"));
    procesarComandoHardware(doc);
    pasoRecibido = "";
}