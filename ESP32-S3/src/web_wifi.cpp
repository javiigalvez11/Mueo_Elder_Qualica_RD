#include "web_wifi.hpp"


// Helper para Content-Type
static String contentTypeFromPath(const String &path)
{
    if (path.endsWith(".html"))
        return "text/html; charset=utf-8";
    if (path.endsWith(".css"))
        return "text/css";
    if (path.endsWith(".js"))
        return "application/javascript";
    if (path.endsWith(".png"))
        return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
        return "image/jpeg";
    if (path.endsWith(".svg"))
        return "image/svg+xml";
    if (path.endsWith(".ico"))
        return "image/x-icon";
    if (path.endsWith(".json"))
        return "application/json";
    return "application/octet-stream";
}

// ========================= Autenticación y Redirección =========================

static void redirectStatusWiFi(const String &kind, const String &title, const String &msg,
                               const String &backUrl, const String &backText,
                               const String &redirUrl = "", int redirMs = 0)
{
    String loc = "/status?kind=" + kind + "&title=" + title + "&msg=" + msg + "&back=" + backUrl + "&backText=" + backText;
    if (redirUrl.length() > 0 && redirMs > 0)
    {
        loc += "&redir=" + redirUrl + "&ms=" + String(redirMs);
    }
    serverWiFi.sendHeader("Location", loc);
    serverWiFi.send(302, "text/plain", "");
}

static inline bool requireAuthWiFi()
{
    if (!registrado_eth)
    {
        serverWiFi.sendHeader("Location", "/");
        serverWiFi.send(302, "text/plain", "");
        return false;
    }
    lastActivityTime_eth = millis();
    return true;
}

// ========================= Handlers de Estado y UI =========================
void handleWiFiRoot()
{
    File file = LittleFS.open("/index.html", "r");
    if (!file)
    {
        // Mensaje de error crítico si falta el sistema de archivos
        serverWiFi.send(500, "text/plain", "Error critico: No se encuentra index.html en la memoria interna.");
        return;
    }
    String html = file.readString();
    file.close();

    html.replace("{{ERROR_MSG}}", errorMessage_eth);
    errorMessage_eth = "";
    serverWiFi.send(200, "text/html; charset=utf-8", html);
}

// ========================= Handlers de Estado y UI =========================

void handleWiFiStatus()
{
    File file = LittleFS.open("/status.html", "r");
    if (!file)
    {
        serverWiFi.send(500, "text/plain", "Error critico: status.html no encontrado.");
        return;
    }
    String html = file.readString();
    file.close();

    String kind = serverWiFi.arg("kind");
    if (kind != "success" && kind != "error" && kind != "info")
        kind = "info";

    // Título profesional basado en el tipo de mensaje
    String title = serverWiFi.arg("title");
    if (title.length() == 0)
    {
        title = (kind == "success") ? "Operación Exitosa" : (kind == "error") ? "Error en el Proceso"
                                                                              : "Información del Sistema";
    }

    // Mensaje descriptivo por defecto
    String msg = serverWiFi.arg("msg");
    if (msg.length() == 0)
        msg = "La tarea se ha completado correctamente.";

    String back = serverWiFi.arg("back");
    if (back.length() == 0)
        back = "/menu";

    // Texto del botón Volver claro
    String backText = serverWiFi.arg("backText");
    if (backText.length() == 0 || backText == "Volver" || backText == "BACK_ID")
        backText = "Volver al Menú";

    html.replace("{{KIND}}", htmlEscape(kind));
    html.replace("{{TITLE}}", htmlEscape(title));
    html.replace("{{MESSAGE}}", htmlEscape(msg));
    html.replace("{{BACK_URL}}", htmlEscape(back));
    html.replace("{{BACK_TEXT}}", htmlEscape(backText));
    html.replace("{{AUTO_REDIRECT_URL}}", htmlEscape(serverWiFi.arg("redir")));
    html.replace("{{AUTO_REDIRECT_MS}}", htmlEscape(serverWiFi.arg("ms")));

    // Texto de ayuda para la redirección automática
    if (serverWiFi.arg("redir").length() > 0 && serverWiFi.arg("ms").toInt() > 0)
    {
        html.replace("{{REDIRECT_HINT}}", "Redirigiendo automáticamente, espere un momento...");
    }
    else
    {
        html.replace("{{REDIRECT_HINT}}", "");
    }

    serverWiFi.send(200, "text/html; charset=utf-8", html);
}

void handleWiFiSubmit()
{
    if (serverWiFi.arg("password") == credencialMaestra)
    {
        registrado_eth = true;
        lastActivityTime_eth = millis();
        logbuf_clear();
        logbuf_enable(true);
        serverWiFi.sendHeader("Location", "/menu");
        serverWiFi.send(302, "text/plain", "");
    }
    else
    {
        // Mensaje directo en lugar de AUTH_ERROR_ID
        errorMessage_eth = "<span style='color:red;'>Contraseña incorrecta. Por favor, inténtelo de nuevo.</span>";
        serverWiFi.sendHeader("Location", "/");
        serverWiFi.send(302, "text/plain", "");
    }
}

// ========================= Configuración de Red =========================

void handleWiFiConfigPage()
{
    if (!requireAuthWiFi())
        return;

    File file = LittleFS.open("/config.html", "r");
    if (!file)
    {
        // Error descriptivo si falta el archivo en LittleFS
        redirectStatusWiFi("error", "Error de Sistema", "No se ha encontrado el archivo de interfaz config.html", "/menu", "Volver al Menú");
        return;
    }
    String html = file.readString();
    file.close();

    TornoConfig c = cfgLoad();
    String currentIP, currentGW, currentMask, currentDNS1, currentDNS2;

    if (c.modoRed == 0)
    {
        currentIP = WiFi.localIP().toString();
        currentGW = WiFi.gatewayIP().toString();
        currentMask = WiFi.subnetMask().toString();
        currentDNS1 = WiFi.dnsIP(0).toString();
        currentDNS2 = WiFi.dnsIP(1).toString();
    }
    else
    {
        currentIP = c.ip;
        currentGW = c.gw;
        currentMask = c.mask;
        currentDNS1 = c.dns1;
        currentDNS2 = c.dns2;
    }

    html.replace("{{DEVICE_ID}}", htmlEscape(c.deviceId));
    html.replace("{{WIFI_SSID}}", htmlEscape(c.wifiSSID));
    html.replace("{{WIFI_PASS}}", htmlEscape(c.wifiPass));

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", c.mac[0], c.mac[1], c.mac[2], c.mac[3], c.mac[4], c.mac[5]);
    html.replace("{{MAC_ADDRESS}}", String(macStr));

    html.replace("{{IP}}", currentIP);
    html.replace("{{GATEWAY}}", currentGW);
    html.replace("{{SUBNET}}", currentMask);
    html.replace("{{DNS1}}", currentDNS1);
    html.replace("{{DNS2}}", currentDNS2);

    html.replace("{{MOD_VEGA_SEL}}", (c.modoPasillo == 0) ? "selected" : "");
    html.replace("{{MOD_CANOPU_SEL}}", (c.modoPasillo == 1) ? "selected" : "");
    html.replace("{{MOD_ARTURUS_SEL}}", (c.modoPasillo == 2) ? "selected" : "");

    html.replace("{{SENT_LEFT_SEL}}", (c.sentidoApertura == 0) ? "selected" : "");
    html.replace("{{SENT_RIGHT_SEL}}", (c.sentidoApertura == 1) ? "selected" : "");
    html.replace("{{MODO_RS485_SEL}}", (c.modoApertura == 0) ? "selected" : "");
    html.replace("{{MODO_RELES_SEL}}", (c.modoApertura == 1) ? "selected" : "");

    html.replace("{{MODO_DHCP_SEL}}", (c.modoRed == 0) ? "selected" : "");
    html.replace("{{MODO_ESTATICA_SEL}}", (c.modoRed == 1) ? "selected" : "");
    html.replace("{{CON_WIFI_SEL}}", (c.conexionRed == 0) ? "selected" : "");
    html.replace("{{CON_ETH_SEL}}", (c.conexionRed == 1) ? "selected" : "");

    html.replace("{{URL_BASE}}", htmlEscape(c.urlBase));
    html.replace("{{URL_ACTUALIZA}}", htmlEscape(c.urlActualiza));

    html.replace("{{CFG_MSG}}", errorMessage_eth);
    errorMessage_eth = "";
    serverWiFi.send(200, "text/html; charset=utf-8", html);
}

void handleWiFiConfigSave()
{
    if (!requireAuthWiFi())
        return;

    TornoConfig c = cfgLoad();
    c.deviceId = serverWiFi.arg("deviceId");
    c.conexionRed = (uint8_t)serverWiFi.arg("conexionRed").toInt();

    if (serverWiFi.hasArg("wifiSSID"))
    {
        c.wifiSSID = serverWiFi.arg("wifiSSID");
    }
    if (serverWiFi.hasArg("wifiPass"))
    {
        c.wifiPass = serverWiFi.arg("wifiPass");
    }

    c.modoPasillo = (uint8_t)serverWiFi.arg("modoPasillo").toInt();
    c.modoApertura = (uint32_t)serverWiFi.arg("modoApertura").toInt();
    c.sentidoApertura = (uint8_t)serverWiFi.arg("sentidoApertura").toInt();
    c.modoRed = (uint8_t)serverWiFi.arg("modoRed").toInt();
    c.ip = serverWiFi.arg("ip");
    c.gw = serverWiFi.arg("gw");
    c.mask = serverWiFi.arg("mask");
    c.dns1 = serverWiFi.arg("dns1");
    c.dns2 = serverWiFi.arg("dns2");
    c.urlBase = serverWiFi.arg("urlBase");
    c.urlActualiza = serverWiFi.arg("urlActualiza");

    bool valid = isDeviceIdOK(c.deviceId) && c.urlBase.startsWith("http");
    String reason = "Datos de identificación o URL base no válidos.";

    if (c.modoRed == 1 && (!isIPv4OK(c.ip) || !isIPv4OK(c.gw)))
    {
        valid = false;
        reason = "La dirección IP o Puerta de Enlace no tienen un formato válido.";
    }

    if (c.conexionRed == 0 && c.wifiSSID.length() < 1)
    {
        valid = false;
        reason = "El nombre de la red WiFi (SSID) no puede estar vacío.";
    }

    if (!valid)
    {
        errorMessage_eth = "<span style='color:red;'>Error: " + reason + "</span>";
        serverWiFi.sendHeader("Location", "/config");
        serverWiFi.send(302, "text/plain", "");
        return;
    }

    if (cfgSave(c))
    {
        // Mensaje de éxito profesional
        redirectStatusWiFi("success", "Configuración Guardada",
                           "Los cambios se han aplicado correctamente. El dispositivo se reiniciará ahora para conectar con los nuevos parámetros.",
                           "/menu", "Ir al Menú", "/menu", 5000);
        delay(150);
        ESP.restart();
    }
}

void handleWiFiScanWiFi()
{
    // Opcional: proteger el escaneo con autenticación para mantener la seguridad
    if (!requireAuthWiFi()) 
        return;

    int n = WiFi.scanNetworks();
    String json = "[";
    
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\", \"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    
    // Limpiamos la memoria del escaneo
    WiFi.scanDelete(); 

    // Enviamos el JSON de vuelta al frontend usando el objeto de servidor WiFi
    serverWiFi.send(200, "application/json; charset=utf-8", json);
}

// ========================= Parámetros Técnicos (RS485) =========================
void handleWiFiGetAllParams()
{
    if (!requireAuthWiFi())
        return;
    TornoParams p = paramsLoad();

    // Construcción manual del JSON (más rápido y usa menos RAM que librerías pesadas)
    String json = "{\"values\":[";
    json += String(p.machineId) + ",";      // 0
    json += String(p.openingMode) + ",";    // 1
    json += String(p.waitTime) + ",";       // 2
    json += String(p.voiceLeft) + ",";      // 3
    json += String(p.voiceRight) + ",";     // 4
    json += String(p.voiceVol) + ",";       // 5
    json += String(p.masterSpeed) + ",";    // 6
    json += String(p.slaveSpeed) + ",";     // 7
    json += String(p.debugMode) + ",";      // 8
    json += String(p.decelRange) + ",";     // 9
    json += String(p.selfTestSpeed) + ",";  // 10
    json += String(p.passageMode) + ",";    // 11
    json += String(p.closeControl) + ",";   // 12
    json += String(p.singleMotor) + ",";    // 13
    json += String(p.language) + ",";       // 14
    json += String(p.irRebound) + ",";      // 15
    json += String(p.pinchSens) + ",";      // 16
    json += String(p.reverseEntry) + ",";   // 17
    json += String(p.turnstileType) + ",";  // 18
    json += String(p.emergencyDir) + ",";   // 19
    json += String(p.motorResist) + ",";    // 20
    json += String(p.intrusionVoice) + ","; // 21
    json += String(p.irDelay) + ",";        // 22
    json += String(p.motorDir) + ",";       // 23
    json += String(p.clutchLock) + ",";     // 24
    json += String(p.hallType) + ",";       // 25
    json += String(p.signalFilter) + ",";   // 26
    json += String(p.cardInside) + ",";     // 27
    json += String(p.tailgateAlarm) + ",";  // 28
    json += String(p.limitDev) + ",";       // 29
    json += String(p.pinchFree) + ",";      // 30
    json += String(p.memoryFree) + ",";     // 31
    json += String(p.slipMaster) + ",";     // 32
    json += String(p.slipSlave) + ",";      // 33
    json += String(p.irLogicMode) + ",";    // 34
    json += String(p.lightMaster) + ",";    // 35
    json += String(p.lightSlave);           // 36 (Último sin coma)
    json += "]}";

    serverWiFi.send(200, "application/json", json);
}

void handleWiFiParamsSave()
{
    if (!requireAuthWiFi())
        return;
    TornoParams p = paramsLoad();
    int id = serverWiFi.arg("id").toInt();
    int val = serverWiFi.arg("val").toInt();

    switch (id)
    {
    case 0:
        p.machineId = val;
        break;
    case 1:
        p.openingMode = val;
        break;
    case 2:
        p.waitTime = val;
        break;
    case 3:
        p.voiceLeft = val;
        break;
    case 4:
        p.voiceRight = val;
        break;
    case 5:
        p.voiceVol = val;
        break;
    case 6:
        p.masterSpeed = val;
        break;
    case 7:
        p.slaveSpeed = val;
        break;
    case 8:
        p.debugMode = val;
        break;
    case 9:
        p.decelRange = val;
        break;
    case 10:
        p.selfTestSpeed = val;
        break;
    case 11:
        p.passageMode = val;
        break;
    case 12:
        p.closeControl = val;
        break;
    case 13:
        p.singleMotor = val;
        break;
    case 14:
        p.language = val;
        break;
    case 15:
        p.irRebound = val;
        break;
    case 16:
        p.pinchSens = val;
        break;
    case 17:
        p.reverseEntry = val;
        break;
    case 18:
        p.turnstileType = val;
        break;
    case 19:
        p.emergencyDir = val;
        break;
    case 20:
        p.motorResist = val;
        break;
    case 21:
        p.intrusionVoice = val;
        break;
    case 22:
        p.irDelay = val;
        break;
    case 23:
        p.motorDir = val;
        break;
    case 24:
        p.clutchLock = val;
        break;
    case 25:
        p.hallType = val;
        break;
    case 26:
        p.signalFilter = val;
        break;
    case 27:
        p.cardInside = val;
        break;
    case 28:
        p.tailgateAlarm = val;
        break;
    case 29:
        p.limitDev = val;
        break;
    case 30:
        p.pinchFree = val;
        break;
    case 31:
        p.memoryFree = val;
        break;
    case 32:
        p.slipMaster = val;
        break;
    case 33:
        p.slipSlave = val;
        break;
    case 34:
        p.irLogicMode = val;
        break;
    case 35:
        p.lightMaster = val;
        break;
    case 36:
        p.lightSlave = val;
        break;
    }

    paramsSave(p);
    paramsApplyToGlobals(p);
    if (RS485::setParam(MACHINE_ID, (uint8_t)id, (uint16_t)val))
    {
        delay(150);
        RS485::resetDevice(MACHINE_ID);
        serverWiFi.send(200, "application/json", "{\"status\":\"ok\"}");
    }
    else
    {
        // Mensaje de error más descriptivo para el bus RS485
        serverWiFi.send(200, "application/json", "{\"status\":\"error\",\"msg\":\"Error de comunicación: El bus RS485 no responde.\"}");
    }
}

// ========================= Acciones y Estado =========================

void handleWiFiActionsPage()
{
    if (!requireAuthWiFi())
        return;
    File file = LittleFS.open("/actions.html", "r");
    if (!file)
    {
        serverWiFi.send(404);
        return;
    }
    String html = file.readString();
    file.close();
    html.replace("{{DEVICE_ID}}", htmlEscape(cfgLoad().deviceId));
    html.replace("{{RS485_CLASS}}", (modoApertura == 0) ? "" : "hidden");
    serverWiFi.send(200, "text/html; charset=utf-8", html);
}

void handleWiFiDoAction()
{
    if (!requireAuthWiFi())
        return;

    String cmd = serverWiFi.arg("cmd");
    int valP = serverWiFi.arg("p").toInt();
    uint8_t numPeatones = (valP > 0) ? (uint8_t)valP : 1;
    uint8_t m = MACHINE_ID;

    if (modoApertura == 0)
    { // RS485
        if (cmd == "open_r")
        {
            if (sentidoApertura == 0)
                RS485::leftOpen(m, numPeatones);
            else
                RS485::rightOpen(m, numPeatones);
        }
        else if (cmd == "open_l")
        {
            if (sentidoApertura == 0)
                RS485::rightOpen(m, numPeatones);
            else
                RS485::leftOpen(m, numPeatones);
        }
        else if (cmd == "close")
        {
            RS485::closeGate(m);
        }
        else if (cmd == "reset_r")
        {
            if (sentidoApertura == 0)
                RS485::resetLeftCount(m);
            else
                RS485::resetRightCount(m);
        }
        else if (cmd == "reset_l")
        {
            if (sentidoApertura == 0)
                RS485::resetRightCount(m);
            else
                RS485::resetLeftCount(m);
        }
        else if (cmd == "reset_full")
        {
            RS485::resetDevice(m);
        }
        else if (cmd == "emergency")
        {
            // Enviamos comandos al RS485
            RS485::leftAlwaysOpen(m);
            delay(200); // Pequeña pausa para no saturar el bus RS485
            RS485::rightAlwaysOpen(m);

            logbuf_pushf("[SISTEMA] ¡EMERGENCIA ACTIVADA! Puertas abiertas.");
        }
        // --- RESTRICCIONES RS485 ---
        else if (cmd == "forbid_r")
        {
            if (sentidoApertura == 0)
                RS485::forbiddenLeftPassage(m);
            else
                RS485::forbiddenRightPassage(m);
        }
        else if (cmd == "forbid_l")
        {
            if (sentidoApertura == 0)
                RS485::forbiddenRightPassage(m);
            else
                RS485::forbiddenLeftPassage(m);
        }
        else if (cmd == "unrestrict")
        {
            RS485::disablePassageRestriccion(m);
        }
    }
    else // MODO RELES
    {
        if (cmd == "open_r") // Entrada
        {
            (sentidoApertura == 0) ? rele::openEntry() : rele::openExit();
        }
        else if (cmd == "open_l") // Salida
        {
            (sentidoApertura == 0) ? rele::openExit() : rele::openEntry();
        }
        else if (cmd == "close")
        {
            rele::close();
        }
        else if (cmd == "reset_r")
        {
            (sentidoApertura == 0) ? salidasTotales = 0 : entradasTotales = 0;
        }
        else if (cmd == "reset_l")
        {
            (sentidoApertura == 0) ? entradasTotales = 0 : salidasTotales = 0;
        }
    }
    serverWiFi.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleWiFiGetStatusJson()
{
    if (!requireAuthWiFi())
        return;

    String json = "{";

    if (modoApertura == 0) // MODO RS485
    {
        RS485::StatusFrame st = RS485::getStatus();

        // Mapeo para la Web (cnt_der = Entrada, cnt_izq = Salida)
        if (sentidoApertura == 0)
        {
            json += "\"cnt_der\":" + String(st.leftCount) + ",\"cnt_izq\":" + String(st.rightCount);
        }
        else
        {
            json += "\"cnt_der\":" + String(st.rightCount) + ",\"cnt_izq\":" + String(st.leftCount);
        }

        json += ",\"gate_text\":\"Activo\",\"fault_text\":\"OK\",\"alarm_text\":\"Ninguna\"";
    }
    else // MODO RELES
    {
        if (sentidoApertura == 0)
        {
            json += "\"cnt_der\":" + String(entradasTotales) + ",\"cnt_izq\":" + String(salidasTotales);
        }
        else
        {
            json += "\"cnt_der\":" + String(salidasTotales) + ",\"cnt_izq\":" + String(entradasTotales);
        }
        json += ",\"gate_text\":\"Modo Relé\",\"fault_text\":\"N/A\",\"alarm_text\":\"N/A\"";
    }

    json += "}";
    serverWiFi.send(200, "application/json", json);
}

// ========================= Sistema (Logs, Reinicio, Firmware) =========================

void handleWiFiLogsData()
{
    if (!registrado_eth)
    {
        serverWiFi.send(401);
        return;
    }
    uint32_t since = (uint32_t)serverWiFi.arg("since").toInt();
    uint32_t next = 0;
    serverWiFi.send(200, "application/json", logbuf_get_json_since(since, next));
}

void handleWiFiReiniciarDo()
{
    if (!requireAuthWiFi())
        return;
    redirectStatusWiFi("info", "Reiniciando", "El dispositivo se está reiniciando...", "/menu", "Menú");
    delay(200);
    ESP.restart();
}

void handleWiFiFirmwareUploadDo()
{
    if (!registrado_eth)
        return;
    static bool _abortUpload = false;

    HTTPUpload &upload = serverWiFi.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        _abortUpload = false;
        String filename = upload.filename;

        int idx = filename.lastIndexOf('/');
        if (idx >= 0)
            filename = filename.substring(idx + 1);
        idx = filename.lastIndexOf('\\');
        if (idx >= 0)
            filename = filename.substring(idx + 1);

        String expectedName = String(DEVICE_ID) + ".bin";

        if (filename != expectedName)
        {
            _abortUpload = true;
            return;
        }

        size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace))
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (_abortUpload)
            return;
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            Update.printError(Serial);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (_abortUpload)
        {
            // Error de nombre de archivo
            errorMessage_eth = "<span style='color:red;'>El archivo debe llamarse exactamente: " + String(DEVICE_ID) + ".bin</span>";
            return;
        }

        if (!Update.end(true))
        {
            errorMessage_eth = "<span style='color:red;'>Fallo al finalizar la actualización del firmware.</span>";
        }
    }
}

void handleWiFiFsUploadDoCorrect()
{
    if (!registrado_eth)
        return;

    static File fsUploadFile;
    HTTPUpload &upload = serverWiFi.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
            filename = "/" + filename;
        fsUploadFile = LittleFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
            fsUploadFile.close();
    }
}

// ========================= SETUP RUTAS =========================

void setupWebWiFi()
{
    serverWiFi.on("/", HTTP_GET, handleWiFiRoot);
    serverWiFi.on("/menu", HTTP_GET, []()
                  {
        if(!requireAuthWiFi()) return;
        File f = LittleFS.open("/menu.html", "r");
        if(f) { serverWiFi.streamFile(f, "text/html"); f.close(); } else serverWiFi.send(404); });
    serverWiFi.on("/config", HTTP_GET, handleWiFiConfigPage);
    serverWiFi.on("/actions", HTTP_GET, handleWiFiActionsPage);
    serverWiFi.on("/params", HTTP_GET, []()
                  {
        if(!requireAuthWiFi() || modoApertura != 0) { serverWiFi.send(403); return; }
        File f = LittleFS.open("/params.html", "r");
        if(f) { serverWiFi.streamFile(f, "text/html"); f.close(); } else serverWiFi.send(404); });

    serverWiFi.on("/get_all_params", HTTP_GET, handleWiFiGetAllParams);
    serverWiFi.on("/params_save", HTTP_POST, handleWiFiParamsSave);
    serverWiFi.on("/config_save", HTTP_POST, handleWiFiConfigSave);
    serverWiFi.on("/do_action", HTTP_GET, handleWiFiDoAction);
    serverWiFi.on("/get_status_json", HTTP_GET, handleWiFiGetStatusJson);
    // --- NUEVA RUTA PARA EL ESCANEO WIFI ---
    serverWiFi.on("/scan_wifi", HTTP_GET, handleWiFiScanWiFi);
    serverWiFi.on("/logs", HTTP_GET, []()
                  {
        if(!requireAuthWiFi()) return;
        File f = LittleFS.open("/logs.html", "r");
        if(f) { serverWiFi.streamFile(f, "text/html"); f.close(); } else serverWiFi.send(404); });
    serverWiFi.on("/logs_data", HTTP_GET, handleWiFiLogsData);
    serverWiFi.on("/status", HTTP_GET, handleWiFiStatus);
    serverWiFi.on("/submit", HTTP_POST, handleWiFiSubmit);
    serverWiFi.on("/reiniciar", HTTP_GET, []()
                  {
        if(!requireAuthWiFi()) return;
        File f = LittleFS.open("/reiniciar.html", "r");
        if(f) { serverWiFi.streamFile(f, "text/html"); f.close(); } else serverWiFi.send(404); });
    serverWiFi.on("/reiniciar_dispositivo", HTTP_POST, handleWiFiReiniciarDo);
    serverWiFi.on("/logout", HTTP_GET, []()
                  { registrado_eth = false; serverWiFi.sendHeader("Location", "/"); serverWiFi.send(302); });

    // Rutas con Upload
    serverWiFi.on("/upload_firmware", HTTP_GET, []()
                  {
        if(!requireAuthWiFi()) return;
        File f = LittleFS.open("/upload.html", "r");
        if(f) { String h = f.readString(); f.close(); h.replace("VERSION_FIRMWARE", enVersion); serverWiFi.send(200, "text/html", h); } else serverWiFi.send(404); });
    serverWiFi.on("/upload_firmware", HTTP_POST, []()
                  { 
        if (Update.hasError()) {
            redirectStatusWiFi("error", "Fallo de Actualización", "No se pudo procesar el archivo de firmware correctamente.", "/upload_firmware", "Reintentar");
        }
        else { 
            redirectStatusWiFi("success", "Actualización Completada", "El nuevo firmware se ha cargado con éxito. El sistema se reiniciará ahora.", "/menu", "Menú Principal", "/menu", 5000); 
            delay(100); 
            ESP.restart(); 
        } }, handleWiFiFirmwareUploadDo);
    serverWiFi.on("/upload_fs", HTTP_GET, []()
                  { if(!requireAuthWiFi()) return;
    File f = LittleFS.open("/upload_fs.html", "r");
    if(f) {
        serverWiFi.streamFile(f, "text/html");
        f.close();
    } else {
        // Si el archivo no existe en LittleFS, lo servimos desde el String en PROGMEM
        // Esto es útil si el sistema de archivos está dañado
    } });
    
    serverWiFi.on("/upload_fs", HTTP_POST, []()
                  { redirectStatusWiFi("success", "Archivo Cargado", "El archivo se ha subido correctamente al sistema de ficheros interno.", "/upload_fs", "Subir otro"); }, handleWiFiFsUploadDoCorrect);

    serverWiFi.onNotFound([]()
                          {
        String path = serverWiFi.uri();

        if (!registrado_eth && path != "/") {
            if (!path.endsWith(".png") && !path.endsWith(".jpg") && !path.endsWith(".ico") && !path.endsWith(".css") && !path.endsWith(".js")) {
                serverWiFi.sendHeader("Location", "/");
                serverWiFi.send(302, "text/plain", "");
                return;
            }
        }

        if (LittleFS.exists(path)) {
            File f = LittleFS.open(path, "r");
            serverWiFi.streamFile(f, contentTypeFromPath(path));
            f.close();
        } else {
            // Error 404 más limpio
            serverWiFi.send(404, "text/plain", "");
        } });
}

void webHandleClientWiFi() { serverWiFi.handleClient(); }